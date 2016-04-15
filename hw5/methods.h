#include <time.h>

/************* SERVER-SIDE: USER TIME TRACKING ************/
 unsigned long long returnClientConnectedtime(int clientID){

 }
 void startSession(Session* session){
 	session->start= clock();
 }
 void endSession(Session* session){
 	session->end =clock();
 }
 clock_t sessionLength(Session* session){
    if( (session->start <0) || (session->end < 0)){
    	return -1;
    }
    return ((session->end)-(session->start));
 }