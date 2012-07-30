/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "DirectoryListing.h"

#include "QueueManager.h"
#include "SearchManager.h"

#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "FilteredFile.h"
#include "BZUtils.h"
#include "CryptoManager.h"
#include "ResourceManager.h"
#include "SimpleXMLReader.h"
#include "User.h"

#ifdef ff
#undef ff
#endif

namespace dcpp {

DirectoryListing::DirectoryListing(const HintedUser& aUser) : 
	hintedUser(aUser), abort(false), root(new Directory(NULL, Util::emptyString, false, false)) 
{
}

DirectoryListing::~DirectoryListing() {
	delete root;
}

UserPtr DirectoryListing::getUserFromFilename(const string& fileName) {
	// General file list name format: [username].[CID].[xml|xml.bz2|DcLst]

	string name = Util::getFileName(fileName);

	// Strip off any extensions
	if(stricmp(name.c_str() + name.length() - 6, ".DcLst") == 0) {
		name.erase(name.length() - 6);
	}

	if(stricmp(name.c_str() + name.length() - 4, ".bz2") == 0) {
		name.erase(name.length() - 4);
	}

	if(stricmp(name.c_str() + name.length() - 4, ".xml") == 0) {
		name.erase(name.length() - 4);
	}

	// Find CID
	string::size_type i = name.rfind('.');
	if(i == string::npos) {
		return UserPtr();
	}

	size_t n = name.length() - (i + 1);
	// CID's always 39 chars long...
	if(n != 39)
		return UserPtr();

	CID cid(name.substr(i + 1));
	if(cid.isZero())
		return UserPtr();

	return ClientManager::getInstance()->getUser(cid);
}

void DirectoryListing::loadFile(const string& name) throw(Exception) {
	string txt;

	// For now, we detect type by ending...
	string ext = Util::getFileExt(name);

	dcpp::File ff(name, dcpp::File::READ, dcpp::File::OPEN);
	if(stricmp(ext, ".bz2") == 0) {
		FilteredInputStream<UnBZFilter, false> f(&ff);
		loadXML(f, false);
	} else if(stricmp(ext, ".xml") == 0) {
		loadXML(ff, false);
	}
}

class ListLoader : public dcpp::SimpleXMLReader::CallBack {
public:
	ListLoader(DirectoryListing* aList, DirectoryListing::Directory* root, bool aUpdating, const UserPtr& aUser) : list(aList), cur(root), base("/"), inListing(false), updating(aUpdating), user(aUser) { 
	}

	~ListLoader() { }

	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name, const string& data);

	const string& getBase() const { return base; }
private:
	DirectoryListing* list;
	DirectoryListing::Directory* cur;
	UserPtr user;

	StringMap params;
	string base;
	bool inListing;
	bool updating;
};

string DirectoryListing::updateXML(const string& xml) {
	MemoryInputStream mis(xml);
	return loadXML(mis, true);
}

string DirectoryListing::loadXML(InputStream& is, bool updating) {
	ListLoader ll(this, getRoot(), updating, getUser());

	dcpp::SimpleXMLReader(&ll).parse(is);

	return ll.getBase();
}

static const string sFileListing = "FileListing";
static const string sBase = "Base";
static const string sGenerator = "Generator";
static const string sDirectory = "Directory";
static const string sIncomplete = "Incomplete";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sTTH = "TTH";

void ListLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	if(list->getAbort()) {
		throw AbortException();
	}

	if(inListing) {
		if(name == sFile) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty())
				return;
			const string& s = getAttrib(attribs, sSize, 1);
			if(s.empty())
				return;
			const string& h = getAttrib(attribs, sTTH, 2);
			if(h.empty()) {
				return;
			}			
			DirectoryListing::File* f = new DirectoryListing::File(cur, n, Util::toInt64(s), h);
			cur->files.push_back(f);
		} else if(name == sDirectory) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty()) {
				throw SimpleXMLException("Directory missing name attribute");
			}
			bool incomp = getAttrib(attribs, sIncomplete, 1) == "1";
			DirectoryListing::Directory* d = NULL;
			if(updating) {
				for(DirectoryListing::Directory::Iter i  = cur->directories.begin(); i != cur->directories.end(); ++i) {
					if((*i)->getName() == n) {
						d = *i;
						if(!d->getComplete())
							d->setComplete(!incomp);
						break;
					}
				}
			}
			if(d == NULL) {
				d = new DirectoryListing::Directory(cur, n, false, !incomp);
				cur->directories.push_back(d);
			}
			cur = d;

			if(simple) {
				// To handle <Directory Name="..." />
				endTag(name, Util::emptyString);
			}
		}
	} else if(name == sFileListing) {
		const string& b = getAttrib(attribs, sBase, 2);
		if(b.size() >= 1 && b[0] == '/' && b[b.size()-1] == '/') {
			base = b;
		}
		StringList sl = StringTokenizer<string>(base.substr(1), '/').getTokens();
		for(StringIter i = sl.begin(); i != sl.end(); ++i) {
			DirectoryListing::Directory* d = NULL;
			for(DirectoryListing::Directory::Iter j = cur->directories.begin(); j != cur->directories.end(); ++j) {
				if((*j)->getName() == *i) {
					d = *j;
					break;
				}
			}
			if(d == NULL) {
				d = new DirectoryListing::Directory(cur, *i, false, false);
				cur->directories.push_back(d);
			}
			cur = d;
		}
		cur->setComplete(true);

		string generator = getAttrib(attribs, sGenerator, 2);
		ClientManager::getInstance()->setGenerator(user, generator);

		inListing = true;

		if(simple) {
			// To handle <Directory Name="..." />
			endTag(name, Util::emptyString);
		}
	}
}

void ListLoader::endTag(const string& name, const string&) {
	if(inListing) {
		if(name == sDirectory) {
			cur = cur->getParent();
		} else if(name == sFileListing) {
			// cur should be root now...
			inListing = false;
		}
	}
}

string DirectoryListing::getPath(const Directory* d) const {
	if(d == root)
		return "";

	string dir;
	dir.reserve(128);
	dir.append(d->getName());
	dir.append(1, '\\');

	Directory* cur = d->getParent();
	while(cur!=root) {
		dir.insert(0, cur->getName() + '\\');
		cur = cur->getParent();
	}
	return dir;
}

void DirectoryListing::download(Directory* aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio) {

	string target = (aDir == getRoot()) ? aTarget : aTarget + aDir->getName() + PATH_SEPARATOR;

	if(!aDir->getComplete()) {
		// folder is not completed (partial list?), so we need to download it first
		QueueManager::getInstance()->addDirectory("", hintedUser, target, prio);
	} else {	
		// First, recurse over the directories
		Directory::List& lst = aDir->directories;
		sort(lst.begin(), lst.end(), Directory::DirSort());
		for(Directory::Iter j = lst.begin(); j != lst.end(); ++j) {
			download(*j, target, highPrio, prio);
		}
		// Then add the files
		File::List& l = aDir->files;
		sort(l.begin(), l.end(), File::FileSort());
		for(File::Iter i = aDir->files.begin(); i != aDir->files.end(); ++i) {
			File* file = *i;
			try {
				download(file, target + file->getName(), false, highPrio, prio);
			} catch(const QueueException&) {
				// Catch it here to allow parts of directories to be added...
			} catch(const FileException&) {
				//..
			}
		}
	}
}

void DirectoryListing::download(const string& aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio) {
	dcassert(aDir.size() > 2);
	dcassert(aDir[aDir.size() - 1] == '\\'); // This should not be PATH_SEPARATOR
	Directory* d = find(aDir, getRoot());
	if(d != NULL)
		download(d, aTarget, highPrio, prio);
}

void DirectoryListing::download(File* aFile, const string& aTarget, bool view, bool highPrio, QueueItem::Priority prio) {
	Flags::MaskType flags = (Flags::MaskType)(view ? (QueueItem::FLAG_TEXT | QueueItem::FLAG_CLIENT_VIEW) : 0);

	QueueManager::getInstance()->add(aTarget, aFile->getSize(), aFile->getTTH(), getHintedUser(), flags);

	if(highPrio || (prio != QueueItem::DEFAULT))
		QueueManager::getInstance()->setPriority(aTarget, highPrio ? QueueItem::HIGHEST : prio);
}

DirectoryListing::Directory* DirectoryListing::find(const string& aName, Directory* current) {
	string::size_type end = aName.find('\\');
	dcassert(end != string::npos);
	string name = aName.substr(0, end);

	Directory::Iter i = std::find(current->directories.begin(), current->directories.end(), name);
	if(i != current->directories.end()) {
		if(end == (aName.size() - 1))
			return *i;
		else
			return find(aName.substr(end + 1), *i);
	}
	return NULL;
}

struct HashContained {
	HashContained(const DirectoryListing::Directory::TTHSet& l) : tl(l) { }
	const DirectoryListing::Directory::TTHSet& tl;
	bool operator()(const DirectoryListing::File::Ptr i) const {
		return tl.count((i->getTTH())) && (DeleteFunction()(i), true);
	}
private:
	HashContained& operator=(HashContained&);
};

struct DirectoryEmpty {
	bool operator()(const DirectoryListing::Directory::Ptr i) const {
		bool r = i->getFileCount() + i->directories.size() == 0;
		if (r) DeleteFunction()(i);
		return r;
	}
};

void DirectoryListing::Directory::filterList(DirectoryListing& dirList) {
		DirectoryListing::Directory* d = dirList.getRoot();

		TTHSet l;
		d->getHashList(l);
		filterList(l);
}

void DirectoryListing::Directory::filterList(DirectoryListing::Directory::TTHSet& l) {
	for(Iter i = directories.begin(); i != directories.end(); ++i) (*i)->filterList(l);
	directories.erase(std::remove_if(directories.begin(),directories.end(),DirectoryEmpty()),directories.end());
	files.erase(std::remove_if(files.begin(),files.end(),HashContained(l)),files.end());
}

void DirectoryListing::Directory::getHashList(DirectoryListing::Directory::TTHSet& l) {
	for(Iter i = directories.begin(); i != directories.end(); ++i) (*i)->getHashList(l);
	for(DirectoryListing::File::Iter i = files.begin(); i != files.end(); ++i) l.insert((*i)->getTTH());
}

int64_t DirectoryListing::Directory::getTotalSize(bool adl) {
	int64_t x = getSize();
	for(Iter i = directories.begin(); i != directories.end(); ++i) {
		if(!(adl && (*i)->getAdls()))
			x += (*i)->getTotalSize(adls);
	}
	return x;
}

size_t DirectoryListing::Directory::getTotalFileCount(bool adl) {
	size_t x = getFileCount();
	for(Iter i = directories.begin(); i != directories.end(); ++i) {
		if(!(adl && (*i)->getAdls()))
			x += (*i)->getTotalFileCount(adls);
	}
	return x;
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
