/// <summary>
/// The program starts with creating a socket 
/// initializes socket listening at localhost with port 80. 
/// The client httprequest is then read and the fields extracted and copied to a struct.
/// Then the httprequeststruct is initialized 
/// the server date is read and written to the struct
/// the httprequest client version number is read an compared with the server
/// if the client version is correct continue to read the file 
/// if the file exists then finalize the http response with file content, modification date and status codes.
/// 
/// once httpresponse string is ready write out to the sockets using the send() function.  
/// connection is closed after sending. 
/// 
/// for further details please refer to inline comments in the code below
/// 
/// </summary>


#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

SOCKET Connections[10];
int ConnectionCounter = 0;

/*
enum to hold possible states of fields in the current loop while
iterating through the headerlines of the Httpgetheader from the client
*/
typedef enum 
{
	reset		     = 0,
	requestype_http  = 1,
	host_http		 = 2,
	useragent_http   = 3,
	accept_http	     = 4,
	accept_lang      = 5,
	accept_enc       = 6,
	conn_type	     = 7
}Headerfields;

/*
struct to hold data extracted fromt the httpgetheader from client
*/
struct httpget
{
	std::string requesttype;
	std::string path;
	std::string httpversion;

	std::string host;

	std::string useragent;
	std::string accept;
	std::string accept_lang;
	std::string accept_enc;

	std::string conn_type;	
};

/*
struct to hold data for the httpresponse header. 
*/
struct S_httpresponse
{
	std::string wbserverversion;		// HTTP version (this case 1.1)
	std::string wbserverstatus;         // 200, 404 or 505
	std::string wbstatustext;			// status OK, Not Found, HTTP Version not supported
	std::string wbdatum;				// the current date
	std::string wbservername;			// name of webserver( In this case "TheBestwebserver", which is indeed the best of the best"
	std::string wblastmod;				// last modified information about html file
	std::string wbcontent_len;			//size of the html
	std::string wbcontenttype;			//
	std::string wbConnectionstate;      //the field connectionstate 
	std::string http_content_buffer;    //the html file is read and flush out to this buffer
};

/*
int ClientHandlerThread(int index)
{
	char buffer[8000];
	while (true)
	{
		recv(Connections[index], buffer, sizeof(buffer), NULL);
		for (int i = 0; i < ConnectionCounter; i++)
		{
			if (i == index)
			continue;
			send(Connections[i], buffer, sizeof(buffer), NULL);
		}
	}
}
*/

int main()
{

////////////////////
/// ////////Standard socket initialization code copied from the MSDN website
	WSADATA wsaData;
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	WORD wVersionRequested = MAKEWORD(2, 2);		
	if (WSAStartup(wVersionRequested, &wsaData) != 0) 
	{
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return 1;
	}
	else
		printf("The Winsock 2.2 dll was found okay\n");

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);	
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");			//hostname localhost
	addr.sin_port = htons(80);									//Port
	addr.sin_family = AF_INET;									//regular networking tcp stuff 
	
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN_HINT(5));							// max lenght of queue of backlog connection.

	SOCKET newConnection;

//////////////////// end of Socket initialization
	
	std::cout << "Server is ready for Connections:" << std::endl;

	while (true)  //keep the server running forever
	{	
		newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen); 

		if (newConnection == 0)
		{
			std::cout << "Failed to accept the client's connection." << std::endl;
		}
		else
		{
			std::cout << "Incoming Connection!" << std::endl;	
			char Browserhead[1024];
			recv(newConnection, Browserhead, sizeof(Browserhead), NULL);
			std::string Browserheadstr = (std::string)Browserhead;
			ConnectionCounter += 1;   //track how many clients tried to connect to server

			std::stringstream masterstream;
			masterstream << Browserheadstr;
			std::string segmentbynewline;
			std::string segmentbyspace;			

			httpget httpgetstruct;

			//this while loop is used to traverse each line of the httpGetrequest and extract data then save them in struct
			while (std::getline(masterstream, segmentbynewline, '\n'))
			{				
				std::stringstream slavestream_t1;
				slavestream_t1 << segmentbynewline;

				int currentfield = reset;    //track current line
				bool firsttrip = true;       //to ensure spaces between text in cases there are multiple fields in a single line. 

				//this while loop is to check which line we are at and then copy the lable and value so that we have 
				//associative values for each element.
				while (std::getline(slavestream_t1, segmentbyspace, ' '))
				{							
					//A switch is used to filter through each line. the default statement tries to determine what line we are at by looking
					// at the first word of the line. 
					// then a case number is allocated for that line so the next time the loop enters the data can be copied to the struct
					switch (currentfield) 
					{
						case requestype_http:  	if (currentfield == requestype_http && segmentbyspace.at(0) == '/')
												{
													httpgetstruct.path = segmentbyspace;
												}
												if (currentfield == requestype_http && segmentbyspace.find("HTTP") != std::string::npos)
												{
													httpgetstruct.httpversion = segmentbyspace;
												}
												break;

						case host_http:			httpgetstruct.host = segmentbyspace;											
												break;

						case useragent_http:    if (firsttrip)
												{
													httpgetstruct.useragent = segmentbyspace;
													firsttrip = false;
												}
												else
												{
													httpgetstruct.useragent.append(" ");
													httpgetstruct.useragent.append(segmentbyspace);
												}							
												break;

						case accept_http:		httpgetstruct.accept = segmentbyspace;
												break;

						case accept_lang:		httpgetstruct.accept_lang = segmentbyspace;
												break;

						case accept_enc:		if (firsttrip)
												{
													httpgetstruct.accept_enc = segmentbyspace;
													firsttrip = false;
												}
												else
												{
													httpgetstruct.accept_enc.append(" ");
													httpgetstruct.accept_enc.append(segmentbyspace);
												}							
												break;

						case conn_type:			httpgetstruct.conn_type = segmentbyspace;
												break;

						default :				if (segmentbyspace == "GET")
												{
													httpgetstruct.requesttype = segmentbyspace;
													currentfield = requestype_http;
												}
												if (segmentbyspace == "Host:")
												{
													currentfield = host_http;
												}
												if (segmentbyspace == "User-Agent:")
												{
													currentfield = useragent_http;
												}
												if (segmentbyspace == "Accept:")
												{
													currentfield = accept_http;
												}
												if (segmentbyspace == "Accept-Language:")
												{
													currentfield = accept_lang;
												}
												if (segmentbyspace == "Accept-Encoding:")
												{
													currentfield = accept_enc;
												}
												if (segmentbyspace == "Connection:")
												{
													currentfield = conn_type;
												}
												break; 
						}
				}
			}

			// THis is to test out the HTTPget header values. 
			std::cout << "===========================================" << std::endl;
			std::cout << "Client says:" << std::endl;
			std::cout << "requesttype: " << httpgetstruct.requesttype << std::endl;
			std::cout << "path: " << httpgetstruct.path << std::endl;
			std::cout << "httpversion: " << httpgetstruct.httpversion << std::endl;
			std::cout << "Host: " << httpgetstruct.host << std::endl;
			std::cout << "user agent: " << httpgetstruct.useragent << std::endl;
			std::cout << "accept: " << httpgetstruct.accept << std::endl;
			std::cout << "accept lang: " << httpgetstruct.accept_lang << std::endl;
			std::cout << "accept encoding: " << httpgetstruct.accept_enc << std::endl;
			std::cout << "connection : " << httpgetstruct.conn_type << std::endl;
			std::cout << "===========================================" << std::endl;
			std::cout << std::endl;

	//prepare httpresponse to send
			S_httpresponse httpresponse; // the httpresponse struct
			
			// the last modification field is only present in the 200 OK state. so initialize at empty string
			// and if everything goes well assign value to it
			httpresponse.wblastmod = " "; 

			//get the current date and time. this is the server time(which is also the client time when run on 127.0.0.1)
			time_t rawtime;
			struct tm * timeinfo;
			char strfbuffer[80];
			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(strfbuffer, 80, "%a, %d %b %Y %H:%M:%S %Z", timeinfo); // time formatted properly for the httpresponse.
			
			httpresponse.wbdatum = strfbuffer;
			httpresponse.wbservername = "TheBestServer/1.0.0 (Win32)";	//this will not change so hard code it(whats true doesnt have to change:-))
			httpresponse.wbcontenttype = "text/html";	//we are hosting html files so this is appropiate and will not change
			//the socket is closed after the response is sent. so put this in this field so that the client doesn't expect anything more.
			httpresponse.wbConnectionstate = "Closed";	

	//check version of browser http
						
			//if the http version doesnt match the server's version then dont continue checking for file
			if (httpgetstruct.httpversion.find("HTTP/1.1") != std::string::npos)
			{
				//this was fetched previously from the httpgetrequest. 
				std::string filepath = (httpgetstruct.path.substr(1, httpgetstruct.path.length()));
				
				//to check file modification time. 
				auto filename = filepath;
				struct stat result;
				if (stat(filename.c_str(), &result) == 0)
				{
					auto mod_time = result.st_mtime;	
					struct tm * modtimeinfo;
					char modtimebuff[80];					
					modtimeinfo = localtime(&mod_time);
					strftime(modtimebuff, 80, "%a, %d %b %Y %H:%M:%S %Z", modtimeinfo);
					httpresponse.wblastmod = modtimebuff;
				}
							
				std::ifstream file_reader;
				file_reader.open(filepath);

				// Test to see if the file was opened
				if (file_reader.is_open())
				{
					file_reader.seekg(0, file_reader.end);
					int file_length = file_reader.tellg();
					file_reader.seekg(0, file_reader.beg);

					char * file_buffer = new char[file_length];

					std::cout << "Reading html file: " << file_length << " characters... " << std::endl;
					// read data as a block:
					file_reader.read(file_buffer, file_length);
					std::cout << "read: " << file_reader.gcount() << std::endl;

					file_reader.close();
					// ...buffer contains the entire file...

					std::string temp;
					for (int i = 0; i < file_length; i++)
					{
						temp += file_buffer[i];
					}

					httpresponse.http_content_buffer = temp;				   // the html file content
					httpresponse.wbcontent_len = std::to_string(file_length);  // the length of the file

					delete[] file_buffer;
										
					httpresponse.wbserverversion = "HTTP/1.1";	// Let client know we have the same http version
					httpresponse.wbserverstatus = "200";		//everything went well with file operation so respond 200
					httpresponse.wbstatustext = "OK";					
				}
				else
				{
					std::cout << "Could not open file!" << std::endl;

					//create custom html page when file is not found.
					httpresponse.http_content_buffer =
						" <!DOCTYPE HTML>"
						"<html>"
						"<head>"
						"<title>404 Not Found</title> "
						"</head>"
						"<body>"
						"<h1>Not Found</h1>"
						"<p>The requested URL /t.html was not found on this server.</p>"						
						"</body>"
						"</html> ";
					httpresponse.wbserverversion = "HTTP/1.1";
					httpresponse.wbserverstatus = "404";
					httpresponse.wbstatustext = "Not Found";
					httpresponse.wblastmod = " "; //modification time left empty because file was not found. 
				}
			}
			else //this means that the http version of the browser is not supported
			{
				//create custom content and error number
				httpresponse.http_content_buffer =
					" <!DOCTYPE HTML>"
					"<html>"
					"<head>"
					"<title>505 HTTP Version not supported</title> "
					"</head>"
					"<body>"
					"<h1>HTTP Version not supported</h1>"
					"<p>The server encountered an internal error or misconfiguration and was unable to complete your request.</p>"
					"<p></p>"
					"</body>"
					"</html> ";

				httpresponse.wbserverversion = "HTTP/1.1";
				httpresponse.wbserverstatus = "505";
				httpresponse.wbstatustext = "HTTP Version not supported";
				httpresponse.wblastmod = " ";  // lastmodification time left empty because we didnt check for the file. 
			}

////		compose everything to send to client

			std::string tobesend;

//the struct is copied over to a single string which can be send as one string to the client using send();
//each line ends with CLRF
//Only include "Last-Modified:" field if the state is 200 OK
			tobesend = httpresponse.wbserverversion;
			tobesend += " " + httpresponse.wbserverstatus + " " + httpresponse.wbstatustext + "\r\n";
			tobesend += "Date: " + httpresponse.wbdatum + "\r\n";
			tobesend += "Server: " + httpresponse.wbservername + "\r\n";
			if (httpresponse.wbserverstatus.find("200") != std::string::npos)
			{
				tobesend += "Last-Modified: " + httpresponse.wblastmod + "\r\n";
			}
			tobesend += "Content-Length: " + httpresponse.wbcontent_len + "\r\n";
			tobesend += "Content-Type: " + httpresponse.wbcontenttype + "\r\n";
			tobesend += "Connection: " + httpresponse.wbConnectionstate + "\r\n";	
			tobesend += "\n";
			tobesend += httpresponse.http_content_buffer + "\r\n";
			
//check if the server response is correct 
			std::cout << std::endl;
			std::cout << "===========================================" << std::endl;
			std::cout << "Server says: " << std::endl;
			std::cout << tobesend << std::endl;
			std::cout << std::endl;
			std::cout << "===========================================" << std::endl;
//convert from string to char * 
			const char *tobesendchar = tobesend.c_str();

			send(newConnection, tobesend.c_str(), tobesend.size(), NULL);
			
			closesocket(newConnection);

			//Connections[i] = newConnection;
			//ConnectionCounter += 1;
			//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(i), NULL, NULL);
		}
	}
	system("pause");
	WSACleanup();

	return 0;
}









