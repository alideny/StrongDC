/* 
* Copyright (C) 2003 Twink, spm7@waikato.ac.nz
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of+
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdinc.h"
#include "WebServerManager.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "LogManager.h"
#include "ClientManager.h"
#include "UploadManager.h"
#include "StringTokenizer.h"
#include "ResourceManager.h"
#include "SearchResult.h"

namespace dcpp {

WebServerManager* Singleton<WebServerManager>::instance = NULL;

//static const string WEBSERVER_AREA = "WebServer";

WebServerManager::WebServerManager(void) : started(false), page404(NULL), sended_search(false), searchInterval(10) {
	SettingsManager::getInstance()->addListener(this);
}

WebServerManager::~WebServerManager(void){
	SettingsManager::getInstance()->removeListener(this);
	if(started) Stop();
}

void WebServerManager::Start(){
	if(started) return;
	Lock l(cs);
	started = true;

	socket.listen((short)SETTING(WEBSERVER_PORT));
	socket.addListener(this);
	fire(WebServerListener::Setup());

	page404 = new WebPageInfo(PAGE_404,"");
	pages["/"] = new WebPageInfo(INDEX, "");
	pages["/index.htm"] = new WebPageInfo(INDEX, "");	
	pages["/dlqueue.html"] = new WebPageInfo(DOWNLOAD_QUEUE, "Download Queue");
	pages["/dlfinished.html"] = new WebPageInfo(DOWNLOAD_FINISHED, "Finished Downloads");
	pages["/ulqueue.html"] = new WebPageInfo(UPLOAD_QUEUE, "Upload Queue");
	pages["/ulfinished.html"] = new WebPageInfo(UPLOAD_FINISHED, "Finished Uploads");
	pages["/weblog.html"] = new WebPageInfo(LOG, "Logs");
	pages["/syslog.html"] = new WebPageInfo(SYSLOG, "System Logs");
	pages["/search.html"] = new WebPageInfo(SEARCH, "Search");
	pages["/logout.html"] = new WebPageInfo(LOGOUT, "Logout");

#ifdef _DEBUG  
	//AllocConsole();
	//freopen("con:","w",stdout);
#endif
}

void WebServerManager::Stop() {
	if(!started) return;
	started = false;
	Lock l(cs);

	delete page404;
	page404 = NULL;

	for(WebPages::iterator p = pages.begin(); p != pages.end(); ++p){
		if(p->second != NULL){
			delete p->second;
			p->second = NULL;
		}
	}

#ifdef _DEBUG 
	//FreeConsole();
#endif
	socket.removeListener(this);
	socket.disconnect();
}

void WebServerManager::on(ServerSocketListener::IncomingConnection) throw() {
	WebServerSocket *wss = new WebServerSocket();
	wss->accept(&socket);	
	wss->start();
}

string WebServerManager::getLoginPage(){
	string pagehtml = "";
    pagehtml = "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>";
	pagehtml += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='cs'>";
	pagehtml += "<head>";
	pagehtml += "    	<title>Strongdc++ webserver - Login Page</title>";   
	pagehtml += "	<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
    pagehtml += "<meta http-equiv='pragma' content='no-cache'>";
    pagehtml += "   	<meta http-equiv='cache-control' content='no-cache, must-revalidate'>";
	pagehtml += "	<link rel='stylesheet' href='http://strongdc.sf.net/webserver/strong.css' type='text/css' title='Default styl' media='screen' />";
    pagehtml += "</head>";
    pagehtml += "<body>";
    pagehtml += "<div id='index_obsah'>";
    pagehtml += "<h1>StrongDC++ Webserver</h1>";
    pagehtml += "<div id='index_logo'></div>";
    pagehtml += "	<div id='login'>";
    pagehtml += "		<form method='get' action='index.htm' enctype='multipart/form-data'>";
    pagehtml += "			<p><strong>Username: </strong><input type='text' name='user'  size='10'/></p>";
    pagehtml += "			<p><strong>Password: </strong><input type='password' name='pass' size='10'></p>";
    pagehtml += "			<p><input class='tlacitko' type='submit' value='Login'></p>";
    pagehtml += "		</form>";
    pagehtml += "	</div>";
    pagehtml += "	<div id='paticka'>";
    pagehtml += "		2004-2010 | Big Muscle | <a href='http://strongdc.sf.net/'>StrongDC++</a>";
    pagehtml += "	</div>";
    pagehtml += "</div>";                                
    pagehtml += "</body>";
    pagehtml += "</html>";

	string header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + Util::toString(pagehtml.length()) + "\r\n\r\n";

	return header + pagehtml;
}

string WebServerManager::getPage(const string& file, const string& IP) {
	printf("requested: '%s'\n",file.c_str()); 
	string header = "HTTP/1.0 200 OK\r\n";
	string pagehtml = "";

	WebPageInfo *page = page404;
	WebPages::const_iterator find = pages.find(file);
	if(find != pages.end()) page = find->second;	
    pagehtml = "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.1//EN' 'http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd'>";
    pagehtml += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='cs'>";
    pagehtml += "<head>";
    pagehtml += "    <title>Strongdc++ webserver</title>";
	
    pagehtml += "	<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
    pagehtml += "    <meta http-equiv='pragma' content='no-cache'>";
    pagehtml += "    <meta http-equiv='cache-control' content='no-cache, must-revalidate'>";
	
    pagehtml += "	<link rel='stylesheet' href='http://strongdc.sf.net/webserver/strong.css' type='text/css' title='Default styl' media='screen' />";
    pagehtml += "</head>";
    pagehtml += "<body>";

    pagehtml += "<div id='obsah'>";
    pagehtml += "<h1>StrongDC++ Webserver</h1>";
    pagehtml += "	<div id='menu'>";
    pagehtml += "		<a href='weblog.html'>Webserver Log</a>";
    pagehtml += "		<a href='syslog.html'>System Log</a>";
    pagehtml += "		<a href='ulfinished.html'>Finished Uploads</a>";
    pagehtml += "		<a href='ulqueue.html'>Upload Queue  </a>";
    pagehtml += "		<a href='dlqueue.html'>Download Queue</a>";
    pagehtml += "		<a href='dlfinished.html'>Finished Downloads</a>";
    pagehtml += "		<a href='search.html'>Search</a>";
    pagehtml += "		<a href='logout.html'>Log out</a>";
    pagehtml += "	</div>";

	if((page->id == SEARCH) && (sended_search == true)) {
		//sended_search = false;
		pagehtml += "<META HTTP-EQUIV='Refresh' CONTENT='" + Util::toString(searchInterval / 1000 + 15) + ";URL=search.html?stop=true'>";
	}

	pagehtml += "	<div id='prava'>";
  
	switch(page->id){
		case INDEX:
			pagehtml+="Welcome to the StrongDC++ WebServer";
			break;

		case DOWNLOAD_QUEUE:
			pagehtml+=getDLQueue();
			break;

		case DOWNLOAD_FINISHED:
			pagehtml+=getFinished(false);
			break;

		case UPLOAD_QUEUE:
			pagehtml+=getULQueue();
			break;

		case UPLOAD_FINISHED:
			pagehtml+=getFinished(true);
			break;

		case SEARCH:
			pagehtml+=getSearch();
			break;

		case LOG:
			pagehtml+=getLogs();
			break;

		case SYSLOG:
			pagehtml+=getSysLogs();
			break;

		case LOGOUT: {
			map<string, uint64_t>::iterator i;
			if((i = loggedin.find(IP)) != loggedin.end())
				loggedin.erase(i);

			pagehtml = getLoginPage();
			return pagehtml;
		}
		case PAGE_404:
		default:
			int action = 0;
			if(file == "/shutdown.htm") action = 0;
			else if(file == "/reboot.htm") action = 2;
			else if(file == "/suspend.htm") action = 3;
			else if(file == "/logoff.htm") action = 1;
			else if(file == "/switch.htm") action = 5;
			else { 
				header = "HTTP/1.0 404 Not Found\r\n";
				pagehtml += "Page not found";
				break;
			}

			fire(WebServerListener::ShutdownPC(), action);
			pagehtml += "Request sent to remote PC :)";

	}

    pagehtml += "	</div>";
    pagehtml += "	<div id='ikony'>";
    pagehtml += "			<a href='switch.htm' id='switch'></a>";
    pagehtml += "			<a href='logoff.htm' id='logoff'></a>";
    pagehtml += "			<a href='suspend.htm' id='suspend'></a>";
    pagehtml += "			<a href='reboot.htm' id='reboot'></a>";
    pagehtml += "			<a href='shutdown.htm' id='shutdown'></a>";
    pagehtml += "	</div>";
    pagehtml += "	<div id='paticka'>";
    pagehtml += "		2004-2010 | Big Muscle | StrongDC++";
    pagehtml += "	</div>";
    pagehtml += "</div>";
                                  
    pagehtml += "</body>";
    pagehtml += "</html>";

	header += "Content-Type: text/html\r\nContent-Length: " + Util::toString(pagehtml.length()) + "\r\n\r\n";

	printf("sending: %s\n",(header + pagehtml).c_str());
	return header + pagehtml;
}

string WebServerManager::getLogs(){
	string ret = "";
	ret = "	<h1>Webserver Logs</h1>";
	//ret += LOGTAIL(WEBSERVER_AREA, 10);

	StringMap params;	
	/*params["user"] = user->getFirstNick();	
	params["hub"] = user->getClientName();
	params["mynick"] = user->getClientNick();	
	params["mycid"] = user->getClientCID().toBase32();	
	params["cid"] = user->getCID().toBase32();	
	params["hubaddr"] = user->getClientAddressPort();	*/

	string path = SETTING(LOG_DIRECTORY) + SETTING(LOG_FILE_WEBSERVER);
		
	try {
		File f(path, File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}

		StringList lines = StringTokenizer<string>(f.read(32*1024), "\r\n").getTokens();

		size_t linesCount = lines.size();

		size_t i = linesCount > 11 ? linesCount - 11 : 0;

		for(; i < (linesCount - 1); ++i){
			ret += "<br>" + lines[i];
		}

		f.close();
	} catch(FileException & /*e*/){
	}

/*	string::size_type i = 0;
	while( (i = ret.find('\n', i)) != string::npos) {
		ret.replace(i, 1, "<br>");
		i++;
	}*/

	return ret;
}

string WebServerManager::getSysLogs(){
	string ret = "";
	ret += "<h1>System Logs</h1><br>";

	string path = SETTING(LOG_DIRECTORY) + SETTING(LOG_FILE_SYSTEM);
		
	try {
		File f(path, File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}

		StringList lines = StringTokenizer<string>(f.read(32*1024), "\r\n").getTokens();

		size_t linesCount = lines.size();

		size_t i = linesCount > 11 ? linesCount - 11 : 0;

		for(; i < (linesCount - 1); ++i){
			ret += "<br>" + lines[i];
		}

		f.close();
	} catch(FileException & /*e*/){
	}
	return ret;
}

string WebServerManager::getSearch() {
	string ret = "<form method='GET' name='search' ACTION='search.html' enctype='multipart/form-data'>";

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	const Client::List& clients = clientMgr->getClients();
	sClients.clear();
	
	for(Client::Iter it = clients.begin(); it != clients.end(); ++it) {
		Client* client = it->second;
		if (!client->isConnected())
			continue;
		
		ret += "<br> >>> " + client->getHubName();
		sClients.push_back(client->getHubUrl());
	}
	
	clientMgr->unlock();

	if(sended_search == true) {
		ret += "<br><br><table width='100%'><tr><td align='center'><b>Searching... Please wait.</b></td></tr>";
		ret += "<tr><td align='center'><input type=\"submit\" name=\"stop\" value=\"cancel\"></td></tr></form></table>";
		sended_search = false;
	} else {
		ret += "<br><table width='100%'><tr><td align='center'><b>Search for</b>&nbsp;<input type=text name='search'>";
		ret += "<input type='submit' value='Search'></td></tr>";
		ret += "<tr><td align='center'>";
		ret += "<select name='type'>";
		ret += "<option value='0' selected>" + STRING(ANY);
		ret += "<option value='1'>" + STRING(AUDIO);
		ret += "<option value='2'>" + STRING(COMPRESSED);
		ret += "<option value='3'>" + STRING(DOCUMENT);
		ret += "<option value='4'>" + STRING(EXECUTABLE);
		ret += "<option value='5'>" + STRING(PICTURE);
		ret += "<option value='6'>" + STRING(VIDEO);
		ret += "<option value='7'>" + STRING(DIRECTORY);
		ret += "<option value='8'>TTH";
		ret += "</td></tr></form></table><br>";	
		ret += "<table width='100%' bgcolor='#EEEEEE'>" + results + "</table>";
	}

	return ret;
}

string WebServerManager::getFinished(bool uploads){
	string ret;

	const FinishedItemList& fl = FinishedManager::getInstance()->lockList(uploads);
	ret = "	<h1>Finished ";
	ret += (uploads ? "Uploads" : "Downloads");
	ret += "</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>Time</td>";
	ret += "			<td>Name</td>";
	ret += "			<td>Size</td>";
	ret += "		</tr>";
	for(FinishedItemList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		ret+="<tr>";
		ret+="	<td>" + Util::formatTime("%Y-%m-%d %H:%M:%S", (*i)->getTime()) + "</td>";
		ret+="	<td>" + Util::getFileName((*i)->getTarget()) + "</td>";
		ret+="	<td>" + Util::formatBytes((*i)->getSize()) + "</td>";			
		ret+="</tr>";
	}
	ret+="</table>";
	FinishedManager::getInstance()->unlockList();

	return ret;
}

string WebServerManager::getDLQueue(){
	string ret;

	const QueueItem::StringMap& li = QueueManager::getInstance()->lockQueue();
	ret = "	<h1>Download Queue</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>Name</td>";
	ret += "			<td>Size</td>";
	ret += "			<td>Downloaded</td>";
	ret += "			<td>Speed</td>";
	ret += "			<td>Segments</td>";
	ret += "		</tr>";
	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		double percent = (aQI->getSize()>0)? aQI->getDownloadedBytes() * 100.0 / aQI->getSize() : 0;
		ret += "	<tr>";
		ret += "		<td>" + aQI->getTargetFileName() + "</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getSize()) + "</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getDownloadedBytes()) + " ("+ Util::toString(percent) + "%)</td>";
		ret += "		<td>" + Util::formatBytes(aQI->getAverageSpeed()) + "/s</td>";
		ret += "		<td>" + Util::toString((int)aQI->getDownloads().size())+"/"+Util::toString(aQI->getMaxSegments()) + "</td>";
		ret += "	</tr>";
	}
	ret+="</table>";
	QueueManager::getInstance()->unlockQueue();

	return ret;
}

string WebServerManager::getULQueue(){
	string ret = "";
	ret = "	<h1>Upload Queue</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>User</td>";
	ret += "			<td>Filename</td>";
	ret += "		</tr>";
	UploadQueueItem::SlotQueue users = UploadManager::getInstance()->getUploadQueue();
	for(UploadQueueItem::SlotQueue::const_iterator ii = users.begin(); ii != users.end(); ++ii) {
		for(UploadQueueItem::List::const_iterator i = ii->second.begin(); i != ii->second.end(); ++i) {
			ret+="<tr><td>" + ClientManager::getInstance()->getNicks(ii->first.user)[0] + "</td>";
			ret+="<td>" + Util::getFileName((*i)->getFile()) + "</td></tr>";
		}
	}
	ret+="</table>";
	return ret;
}	

StringMap WebServerSocket::getArgs(const string& arguments) {
	StringMap args;

	string::size_type i = 0;
	string::size_type j = 0;
	while((i = arguments.find("=", j)) != string::npos) {
		string key = arguments.substr(j, i-j);
		string value = arguments.substr(i + 1);;

		j = i + 1;
		if((i = arguments.find("&", j)) != string::npos) {
			value = arguments.substr(j, i-j);
			j = i + 1;
		} 

		args[key] = value;
	}

	return args;
}

int WebServerSocket::run(){
	char buff[8192];
	uint16_t test = 0;
	memzero(buff, sizeof(buff));
	while(true) {

		test++;
		if(test >= 10000)
			break;

		int size = recv(sock,buff,sizeof(buff),0);

		string header = buff;
		header = header.substr(0,size);

		size_t start = 0, end = 0;

		string IP = Util::toString(from.sin_addr.S_un.S_un_b.s_b1) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b2) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b3) + string(".") + Util::toString(from.sin_addr.S_un.S_un_b.s_b4);

		dcdebug("%s\n", header.c_str());	

		if(((start = header.find("GET ")) != string::npos) && (end = header.substr(start+4).find(" ")) != string::npos ){
			if(BOOLSETTING(LOG_WEBSERVER)) {
				StringMap params;
				params["file"] = header.substr(start+4,end);
				params["ip"] = IP;
				LOG(LogManager::WEBSERVER, params);
			}

			header = header.substr(start+4,end);
			bool check = false;
			//dcdebug(header.c_str());
			if((start = header.find("?")) != string::npos) {
				string arguments = header.substr(start+1);
				header = header.substr(0, start);
				StringMap m = getArgs(arguments);

				if(m["user"] == SETTING(WEBSERVER_USER) && m["pass"] == SETTING(WEBSERVER_PASS))
					WebServerManager::getInstance()->login(IP);
				if((m["search"] != Util::emptyString)) {
					WebServerManager::getInstance()->search(Util::encodeURI(m["search"], true), Util::toInt(m["type"]));
				}
				if((m["stop"] != Util::emptyString)) {
					check = m["stop"] == "true";
					WebServerManager::getInstance()->reset();
				}
				if(m["name"] != Util::emptyString) {
					try {
						QueueManager::getInstance()->add(SETTING(DOWNLOAD_DIRECTORY) + Util::encodeURI(m["name"], true), Util::toInt64(m["size"]), TTHValue(m["tth"]), HintedUser(UserPtr(), Util::emptyString));
					} catch(const Exception&) {
						// ...
					}
				}
			}

			string toSend = Util::emptyString;
		
			if(!WebServerManager::getInstance()->isloggedin(IP)) {
				toSend = WebServerManager::getInstance()->getLoginPage();
			} else {
				//if(!check || (WebServerManager::getInstance()->getResults() != Util::emptyString))
					toSend = WebServerManager::getInstance()->getPage(header, IP);
				/*else
					break;*/
			}
	
			::send(sock, toSend.c_str(), toSend.size(), 0);
			break;
		}
	}
	::closesocket(sock);
	delete this;
	return 0;

} 

void WebServerManager::on(SearchManagerListener::SR, const SearchResultPtr& aResult) throw() {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);
		
		if(!aResult->getToken().empty() && token != aResult->getToken()) {
			return;
		}
				
		if(aResult->getType() == SearchResult::TYPE_FILE) {
			results += "<form method='GET' name=\"form" + Util::toString(row) + "\" ACTION='search.html' >";
			//results += "<input type=\"hidden\" name='file' value='" + aResult->getFile() + "'>";
			results += "<input type=\"hidden\" name='size' value='" + Util::toString(aResult->getSize()) + "'>";
			results += "<input type='hidden' name='name' value='" + aResult->getFileName() + "'>";
			results += "<input type='hidden' name='tth' value='" + aResult->getTTH().toBase32() + "'>";
			results += "<input type='hidden' name='type' value='" + Util::toString(aResult->getType()) + "'>";
			results += "<tr onmouseover=\"this.style.backgroundColor='#CC0099'; this.style.cursor='hand';\" onmouseout=\"this.style.backgroundColor='#EEEEEE'; this.style.cursor='hand';\" onclick=\"form" + Util::toString(row) + ".submit();\">";
			results += "<td>" + ClientManager::getInstance()->getNicks(HintedUser(aResult->getUser(), aResult->getHubURL()))[0] + "</td><td>" + aResult->getFileName() + "</td><td>" + Util::formatBytes(aResult->getSize()) + "</td><td>" + aResult->getTTH().toBase32() + "</td></form></tr>";
			row++;
		}
	}

	//SearchInfo* i = new SearchInfo(aResult);
	//PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)i);	
}

} // namespace dcpp
