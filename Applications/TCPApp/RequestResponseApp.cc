//
// Copyright 2004 Andras Varga
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//


#include "RequestResponseApp.h"


#define MSGKIND_CONNECT  0
#define MSGKIND_SEND     1


Define_Module(RequestResponseApp);


void RequestResponseApp::initialize()
{
    TCPGenericCliAppBase::initialize();

    timeoutMsg = new cMessage("timer");

    numRequestsToSend = 0;
    WATCH(numRequestsToSend);

    timeoutMsg->setKind(MSGKIND_CONNECT);
    scheduleAt((simtime_t)par("startTime"), timeoutMsg);
}

void RequestResponseApp::sendRequest()
{
     ev << "sending request, " << numRequestsToSend-1 << " more to go\n";

     long requestLength = par("requestLength");
     long responseLength = par("responseLength");
     if (requestLength<1) requestLength=1;
     if (responseLength<1) responseLength=1;

     sendPacket(requestLength, responseLength);
}

void RequestResponseApp::handleTimer(cMessage *msg)
{
    switch (msg->kind())
    {
        case MSGKIND_CONNECT:
            ev << "starting session\n";
            connect();
            break;

        case MSGKIND_SEND:
           sendRequest();
           numRequestsToSend--;
           // no scheduleAt(): next request will be send when reply to this one
           // has arrived (see socketDataArrived())
           break;
    }
}

void RequestResponseApp::socketEstablished(int connId, void *ptr)
{
    TCPGenericCliAppBase::socketEstablished(connId, ptr);

    // determine number of requests in this session
    numRequestsToSend = (long) par("numRequests");
    if (numRequestsToSend<1) numRequestsToSend=1;

    // perform first request (next one will be sent when reply arrives)
    sendRequest();
    numRequestsToSend--;
}

void RequestResponseApp::socketDataArrived(int connId, void *ptr, cMessage *msg, bool urgent)
{
    TCPGenericCliAppBase::socketDataArrived(connId, ptr, msg, urgent);

    if (numRequestsToSend>0)
    {
        ev << "reply arrived\n";
        timeoutMsg->setKind(MSGKIND_SEND);
        scheduleAt(simTime()+(simtime_t)par("thinkTime"), timeoutMsg);
    }
    else
    {
        ev << "closing session\n";
        close();
    }
}

void RequestResponseApp::socketClosed(int connId, void *ptr)
{
    TCPGenericCliAppBase::socketClosed(connId, ptr);

    // start another session after a delay
    timeoutMsg->setKind(MSGKIND_CONNECT);
    scheduleAt(simTime()+(simtime_t)par("idleInterval"), timeoutMsg);
}

void RequestResponseApp::socketFailure(int connId, void *ptr, int code)
{
    TCPGenericCliAppBase::socketFailure(connId, ptr, code);

    // reconnect after a delay
    timeoutMsg->setKind(MSGKIND_CONNECT);
    scheduleAt(simTime()+(simtime_t)par("reconnectInterval"), timeoutMsg);
}

