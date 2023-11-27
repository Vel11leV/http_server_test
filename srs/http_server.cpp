#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <list>
#include <iterator>


#define HTTP_SERVER_DEBUG_MODE  0  // release mode '0', debug mode '1'
#define HTTP_SERVER_TCP_PORT  3425
#define HTTP_SERVER_DATA_BUF_SIZE  1024


using namespace std;

const string http_server_HTTP = " HTTP/";
const string http_server_GET = "GET /";
const string http_server_POST = "POST /";
const string http_server_DELETE = "DELETE /";
const string http_server_POST_CONTENT_LENGTH = "Content-Length: ";
const string http_server_POST_CONTENT_TYPE = "Content-Type: ";
const string http_server_POST_EMPTY_LINE = "\r\n\r\n";
const string http_server_200_OK = "HTTP/1.1 200 OK\r\n";
const string http_server_404_NOT_FOUND = "HTTP/1.1 404 Not found\r\n";
const string http_server_POST_content_TEXT_PLAIN = " text/plain\r\n";

typedef struct {
	std::string str_uri;
	std::string str_data;
	std::string str_contentType;
	int int_dataVersion;
} uriData_t;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

list<uriData_t> http_server_dataContainerList; //the global data list
int http_server_dataContainerListAdd(uriData_t &uriData); // add or refresh global http_server_dataContainerList container
int http_server_dataContainerListGet(uriData_t &uriData); // find data in the global http_server_dataContainerList container
int http_server_dataContainerListRemove(uriData_t &uriData);
string http_server_dataRespond_200_OK(string data, string contentType);
string http_server_dataRespond_404_NotFound(string data, string contentType);
int http_server_ProcessRequest(string request, string & response);


int main(void)
{
    int sock, listener;
    struct sockaddr_in addr;
    char buf[HTTP_SERVER_DATA_BUF_SIZE];

    int bytes_read;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_SERVER_TCP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);

    #if HTTP_SERVER_DEBUG_MODE 
        std::cout << " HTTP server started. "<< std::endl;
    #endif

    while(1)
    {
        sock = accept(listener, NULL, NULL);
        if(sock < 0)
        {
            perror("accept");
            exit(3);
        }
        while(1)
        {
            bytes_read = recv(sock, buf, 1024, 0);
            if(bytes_read <= 0){
            	break;
			}        
	    	string responce;   
            http_server_ProcessRequest(buf,responce);
            
            char chars[responce.length() + 1];
            responce.copy(chars, responce.length() + 1);
            send(sock, chars, responce.size(), 0);
            #if HTTP_SERVER_DEBUG_MODE 
                printf("========\n"); 
                std::cout << responce<< std::endl;
                printf("========\n"); 
            #endif
            break;
        }    
        close(sock);
    }  
    return 0;
}


/**
* @brief Generall requests processing function
* @param[in] request - Input request string
* @param[out] &response -  Output responce string
* return - 1 in the case of success responce, 0 - otherwise
*/
int http_server_ProcessRequest(string request, string &response)
{
	int retValue = 0;
	if (request.find(http_server_HTTP)!=string::npos)
	{

		#if HTTP_SERVER_DEBUG_MODE 
		    std::cout << " HTTP request detected. "<< std::endl;
		#endif	

		if(request.find(http_server_GET)!=string::npos){		
			 /*URI parsing*/
			 int uriLen = request.find(http_server_HTTP) - (request.find(http_server_GET) +  http_server_GET.size());
			 string uri = request.substr(http_server_GET.size(), uriLen); 
			 
			 uriData_t  l_uriData;
			 l_uriData.str_uri = uri;
			 int getResult = http_server_dataContainerListGet(l_uriData);
			 #if HTTP_SERVER_DEBUG_MODE 
			     std::cout << " GET request. "<< std::endl;	 
			     std::cout << " URI: "<< uri << std::endl;	
			 #endif     
			 if (getResult)	{
			 	response =  http_server_dataRespond_200_OK(l_uriData.str_data, l_uriData.str_contentType);
			 } else {
			 	response = http_server_dataRespond_404_NotFound("","");
			 }

		} else if (request.find(http_server_POST)!=string::npos){
			 /*URI parsing*/
			 int uriLen = request.find(http_server_HTTP) - (request.find(http_server_POST) +  http_server_POST.size());
			 string uri = request.substr(http_server_POST.size(), uriLen);
			 /*Data length parsing*/
			 int dataLenStringLen = request.find(http_server_POST_CONTENT_TYPE) - (request.find(http_server_POST_CONTENT_LENGTH) + http_server_POST_CONTENT_LENGTH.size());
			 string dataLenString = request.substr(request.find(http_server_POST_CONTENT_LENGTH) + http_server_POST_CONTENT_LENGTH.size(), dataLenStringLen);
			 int dataLen = stoi(dataLenString);
			 /*Data parsing*/
             string data = request.substr(request.find(http_server_POST_EMPTY_LINE) + http_server_POST_EMPTY_LINE.size(), dataLen);
             
             /*Data type parsing*/
             int typeStringLength = request.find(http_server_POST_EMPTY_LINE) - (request.find(http_server_POST_CONTENT_TYPE) + http_server_POST_CONTENT_TYPE.size());
             string type = request.substr(request.find(http_server_POST_CONTENT_TYPE)+http_server_POST_CONTENT_TYPE.size(),typeStringLength);
			 			 
			 /*Save or update the data in the global list*/
			 uriData_t  l_uriData;
			 l_uriData.str_uri = uri;
			 l_uriData.str_data = data;
			 l_uriData.str_contentType = type;
			 l_uriData.int_dataVersion = 0;
			 http_server_dataContainerListAdd(l_uriData);
			 #if HTTP_SERVER_DEBUG_MODE 
			     std::cout << " POST request. "<< std::endl;
			     std::cout << " URI: "<< uri << std::endl;
			     std::cout << " Data length (int) :"<< dataLen << std::endl;
			     std::cout << " Data :"<< data << std::endl;
			     std::cout << " Data type :"<< type << std::endl;
			 #endif     
			 response = http_server_dataRespond_200_OK("","");
		} else if (request.find(http_server_DELETE)!=string::npos){
			/*URI parsing*/
			 int uriLen = request.find(http_server_HTTP) - (request.find(http_server_DELETE) +  http_server_DELETE.size());
			 string uri = request.substr(http_server_DELETE.size(), uriLen);
			 uriData_t  l_uriData;
			 l_uriData.str_uri = uri;
			 int removeResult = http_server_dataContainerListRemove(l_uriData);
			 #if HTTP_SERVER_DEBUG_MODE
			     std::cout << " DELETE request. "<< std::endl;	
			     std::cout << " URI: "<< uri << std::endl;
			 #endif 
			 if (removeResult){
			 	response =  http_server_dataRespond_200_OK("", "");	
			 } else {
			 	response = http_server_dataRespond_404_NotFound("","");
			 }
	
		} else {
			 //*response = http_server_dataRespond_404_NotFound("","");
		}			
	} 
	return retValue;
}

/**
* @brief Function that adds or updates the main data list
* @param[in] uriData - Input data
* return - 1 in the case of success responce, 0 - otherwise
*/
int http_server_dataContainerListAdd(uriData_t &uriData)
{
	int added = 0;
	if (http_server_dataContainerList.empty()){
		#if HTTP_SERVER_DEBUG_MODE
		    std::cout << "The list is empty" << std::endl;
		#endif 
		http_server_dataContainerList.push_back(uriData);
		added = 1;
	} else {
		#if HTTP_SERVER_DEBUG_MODE
            std::cout << "The list is not empty" << std::endl; 	
        #endif
        for (std::list<uriData_t>::iterator it=http_server_dataContainerList.begin(); it != http_server_dataContainerList.end(); std::advance(it, 1))
        {
        	if (it->str_uri == uriData.str_uri) {
        		if(it->str_data != uriData.str_data){ // increment the version field if we dedtected the different data that it was 
        			it->int_dataVersion++; // increment version counter 
        			uriData.int_dataVersion = it->int_dataVersion; // update the version counter of the externall structure 
				}	
        		it->str_data = uriData.str_data; // write new data  
        		added = 1;
				break;   		
			} 				        
        }
        if (added != true) { // if the data not previously refreshed 
        	std::list<uriData_t>::iterator it = http_server_dataContainerList.end(); // check the last element
        	
        	if (it->str_uri == uriData.str_uri) {
        		if(it->str_data != uriData.str_data){ // increment the version field if we dedtected the different data that it was 
        			it->int_dataVersion++; // increment version counter 
        			uriData.int_dataVersion = it->int_dataVersion; // update the version counter of the externall structure 
				}	
        		it->str_data = uriData.str_data; // write new data  
        		added = 1;		
			} else {
				http_server_dataContainerList.push_back(uriData);
				added = 1;
			}		
		}
    }
    #if HTTP_SERVER_DEBUG_MODE
    for (std::list<uriData_t>::iterator it=http_server_dataContainerList.begin(); it != http_server_dataContainerList.end(); std::advance(it, 1))
    { 	
    	std::cout <<"uri: "<< it->str_uri << std::endl;
   	    std::cout <<"data: "<< it->str_data << std::endl;
   	    std::cout <<"type:"<< it->str_contentType << std::endl;
   	    std::cout <<"version:"<< it->int_dataVersion << std::endl;  
    }
    #endif
  	return added; 
}

/**
* @brief Function that gets the data from the global data list
* @param[in/out] &uriData - The place for the data (reading URI and updating the values)
* return - 1 in the case of data found success, 0 - if no input URI data found
*/
int http_server_dataContainerListGet(uriData_t &uriData)
{
	int retVal = 0;
	for (std::list<uriData_t>::iterator it=http_server_dataContainerList.begin(); it != http_server_dataContainerList.end(); std::advance(it, 1))
    { 	
        if (it->str_uri == uriData.str_uri){
            uriData.str_data = it->str_data;
            uriData.int_dataVersion =it->int_dataVersion;
            uriData.str_contentType = it->str_contentType;
            retVal = 1;
            break;	
		}
    }
   
    if (retVal != 1) {
    	std::list<uriData_t>::iterator it=http_server_dataContainerList.end();
    	if (it->str_uri == uriData.str_uri){
            uriData.str_data = it->str_data;
            uriData.int_dataVersion = it->int_dataVersion;
            uriData.str_contentType = it->str_contentType;
            retVal = 1;
		} 
	}	  
	return retVal;
}


/**
* @brief Removes the data with the defenit URI from the global data list
* @param[in] &uriData - reading URI 
* return - 1 if removed successfully , 0 - if no input URI data found
*/
int http_server_dataContainerListRemove(uriData_t &uriData){	
	int retVal = 0;	
	for (std::list<uriData_t>::iterator it=http_server_dataContainerList.begin(); it != http_server_dataContainerList.end(); std::advance(it, 1))
    { 	
        if (it->str_uri == uriData.str_uri){
            http_server_dataContainerList.erase(it);
            retVal = 1;
            break;	
		}
    }   
    if (retVal != 1) {
    	std::list<uriData_t>::iterator it=http_server_dataContainerList.end();
    	if (it->str_uri == uriData.str_uri){
            http_server_dataContainerList.erase(it);
            retVal = 1;
		} else {
			#if HTTP_SERVER_DEBUG_MODE
			    std::cout <<"URI to remove not found: error 404 "<< std::endl; 	//not found 
			#endif

		}
	}  
	if(retVal) {
		#if HTTP_SERVER_DEBUG_MODE
		    std::cout <<"Removing done"<< std::endl;  
		#endif
	}	
	return retVal;	
}


/**
* @brief Function that shapes OK responce 
* @param[in] data - Data
* @param[in] contentType - data content type
* return - HTTP OK responce
*/
string http_server_dataRespond_200_OK(string data, string contentType)
{
	string retRespond;
	if (!data.empty() && !contentType.empty()) {
		retRespond = http_server_200_OK + http_server_POST_CONTENT_TYPE + contentType + "\r\n"
	             + http_server_POST_CONTENT_LENGTH + to_string(data.size()) + http_server_POST_EMPTY_LINE + data +"\r\n";
	} else {
		retRespond = http_server_200_OK;
	}	
	return retRespond;
}

/**
* @brief Function that shapes 404 responce 
* @param[in] data - Data
* @param[in] contentType - data content type
* return - HTTP 404 responce
*/
string http_server_dataRespond_404_NotFound(string data, string contentType)
{
	string retRespond;
	if (!data.empty() && !contentType.empty()) {
	    retRespond = http_server_404_NOT_FOUND + http_server_POST_CONTENT_TYPE + contentType + "\r\n"
	             + http_server_POST_CONTENT_LENGTH + to_string(data.size()) + http_server_POST_EMPTY_LINE + data +"\r\n";
	} else {
		retRespond = http_server_404_NOT_FOUND;
	}
	return retRespond;
}

