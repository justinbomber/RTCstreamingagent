# Author: Philip Chang
# Date:2023/11/16
import sys
import ssl
import json
import asyncio
import logging
import time
import websockets
import threading
import subprocess
import os

logger = logging.getLogger('websockets')
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler(sys.stdout))

clients = {}  # 存儲所有客戶端的 WebSocket 連接
dict480 = {}  # 存儲所有 480 url
dict_token2source={}
online_dict={}
clientgroup=["ddsagent","nchcai","client","admin","hlsproxy","rtspproxy"]
token = set()
#pid480={}
def call480(rtspurl,source):
    #proc = subprocess.Popen(["./../../live/testProgs/testRTSPClient",rtspurl," & echo $!"])
    proc = str(["./../live/testProgs/testRTSPClient",rtspurl," & echo $!"])
    print (proc)
   # proc=subprocess.call(["./../../live/testProgs/testRTSPClient",rtspurl], stderr=subprocess.STDOUT)
    #pid480[source]=(proc.pid)
def kill480(pidnum):
    #os.kill(pidnum, 9)
    print ("kill")
async def handle_websocket(websocket, path):
    client_id = None
    initial_request = None
    try:
        # 處理客戶端ID和WebSocket URL
        splitted = path.split('/')
        splitted.pop(0)
        client_id = splitted.pop(0)
        local_address = websocket.local_address
        complete_ws_url = f"ws://{local_address[0]}:{local_address[1]}{path}"
        ddsagent = "ddsagent"
        ddsagent480 = "ddsagent480"
        nchcai="nchcai"
        admin="admin"
        hlsproxy="hlsproxy"
        rtspproxy="rtspproxy"
        print(f"Received connection on {complete_ws_url}")
        print(f'Client {client_id} connected --------<<<<<')
        if client_id == None:
            pass
        else:
            clients[client_id] = websocket
            if client_id[0:6]==nchcai:
                online_dict[client_id]=clientgroup[1]
            elif client_id[0:8]==ddsagent:
                online_dict[client_id]=clientgroup[0]
                if client_id[0:11] == ddsagent480:
                    dict480.clear()
                    dict_token2source.clear()
                dict480.clear()
                dict_token2source.clear()
            elif client_id==admin:
                online_dict[client_id]=clientgroup[3]
            elif client_id==hlsproxy:
                online_dict[client_id]=clientgroup[4]
            elif client_id==rtspproxy:
                online_dict[client_id]=clientgroup[5]
            else:
                online_dict[client_id]=clientgroup[2]
        while True:
            data = await clients[client_id].recv()
            message = json.loads(data)
            if ddsagent not in clients and ddsagent480 not in clients:
                dict480.clear()
                dict_token2source.clear()
            #if is client 480 and not in dict480
            print(client_id+":")
            print(message)
            if online_dict[client_id]==clientgroup[2]: #client
                if(online_dict.get(ddsagent)==None):
                    newmessage={client_id +"dissconnect"}
                    try:
                        await clients[client_id].send(json.dumps(newmessage))
                    except:
                        print("error: "+message)
                    break
                if message.get("resolution") == "480" and dict480.get(message.get("partition_device"))==None:
                    #if dict480.get(message.get("partition_device")).get("status")!="asked":
                    if ddsagent480 in clients:
                        dict480[message["partition_device"]] = {"token": message["token"],"status": "asked"}
                        dict_token2source[message["token"]]={message["path"]:message["partition_device"]}
                        if ddsagent480 in clients:
                                await clients[ddsagent480].send(json.dumps(message))
                        else:
                                await clients[ddsagent].send(json.dumps(message))
                    else:
                        await clients[ddsagent].send(json.dumps(message))
                elif message.get("resolution") == "480" :
                    if dict480.get(str(message.get("partition_device")))!="None":
                        returnmessage={}
                        returnmessage["token"]=message["token"]
                        returnmessage["path"]=message["path"]
                        print("dict480:",dict480)
                        returnmessage["url"]=dict480[message["partition_device"]].get("url")
                        await clients[client_id].send(json.dumps(returnmessage))
                else:
                    await clients[ddsagent].send(json.dumps(message))
            elif online_dict[client_id]==clientgroup[0]: #ddsagent
                typestring=message.get("type")
                if typestring=="disconnected":
                    partition_device_string=message.get("partition_device")
                    if partition_device_string in dict480:
                        del dict480[partition_device_string]
                        #kill480(pid480[partition_device_string])
                        #del pid480[partition_device_string]
                else:
                    urlstring=message.get("url")
                    tokenstring=message.get("token")
                    pathstring=message.get("path")
                    if urlstring!=None:
                        if tokenstring in  dict_token2source:
                            if pathstring in dict_token2source[tokenstring]:
                                sourcestring=dict_token2source[tokenstring].get(pathstring)
                                if dict480[sourcestring]["status"]=="asked":
                                    dict480[sourcestring]["url"]=urlstring
                                    dict480[sourcestring]["status"]="online"
                                    dict_token2source[tokenstring].pop(pathstring,None)
                                    call480(urlstring,sourcestring)

                    if(tokenstring not in clients):
                        newmessage={}
                        newmessage["token"]=message["token"]
                        newmessage["type"]="disconnect"
                        try:
                            await clients[ddsagent].send(json.dumps(newmessage))
                        except:
                            print("error: "+message)

                    if urlstring[0:7]=="rtsp://":
                        try:
                            await clients[rtspproxy].send(json.dumps(message))
#                            await clients[message["token"]].send(json.dumps(message))
                        except:
                            print("error: "+message)
                    elif urlstring[0:7]=="http://":
                        try:
                            await clients[hlsproxy].send(json.dumps(message))
                        except:
                            print("error: "+message)
                    else:#None
                        try:
                            await clients[message["token"]].send(json.dumps(message))
                        except:
                            print("error: "+message)

            elif online_dict[client_id]==clientgroup[1]: #NCHCAI
                print("NCHCAI:")
                print(message)
                tokenstring=message.get("token")
                rtspurlstring=message.get("rtsp_url")
                devicepathstring=message.get("device_path")
                toclientmessage={}
                toclientmessage["token"]=tokenstring
                toclientmessage["url"]=rtspurlstring
                toclientmessage["path"]=devicepathstring
                if tokenstring not in clients:
                    tonchcmessage={}
                    tonchcmessage["token"]=tokenstring
                    tonchcmessage["device_path"]=devicepathstring
                    tonchcmessage["Result"]=False
                    tonchcmessage["ResultCode"]="401"
                    tonchcmessage["ResultMsg"]="Client has been Disconnected"
                    try:
                        await clients[ddsagent].send(json.dumps(newmessage))
                    except:
                        print("error: "+message)
                else:
                    try:
                        await clients[message["token"]].send(json.dumps(toclientmessage))
                    except:
                        print("error: "+message)
                    if client_id in clients:
                        tonchcmessage={}
                        tonchcmessage["token"]=tokenstring
                        tonchcmessage["device_path"]=devicepathstring
                        tonchcmessage["Result"]=True
                        tonchcmessage["ResultCode"]="200"
                        tonchcmessage["ResultMsg"]="Success"
                        try:
                            await clients[client_id].send(json.dumps(tonchcmessage))
                        except:
                            print("error: "+message)
            elif online_dict[client_id]==clientgroup[3]: # admin
                strtype=message.get("type")
                if strtype=="Disconnect":
                    if ddsagent in clients:
                        try:
                            await clients[ddsagent].close()
                        except:
                            print("error: "+message)
                elif strtype=="close480":
                    closesource=message.get("partition_device")
                    if closesource in dict480:
                        del dict480[closesource]
                        #kill480(pid480[closesource])
                        #del pid480[closesource]
                elif strtype=="set480":
                    startsource=message.get("partition_device")
                    sourceurl=message.get("url")
                    print("geturl",sourceurl)
                    dict480[startsource]={"url":sourceurl}
                    dict480[startsource]["status"]="online"
                    if startsource in dict480:
                        call480(sourceurl,startsource)
                elif strtype=="clearall480":
                    for source in dict480:
                        del dict480[source]
                        #kill480(pid480[source])
                        #del pid480[source]
                elif strtype=="test":
                    if hlsproxy in clients:
                        try:
                            await clients[hlsproxy].send(json.dumps(message))
                        except:
                            print("error: "+message)
            elif online_dict[client_id]==clientgroup[4]: #hlsproxy
                if message["token"] in clients:
                    try:
                        print(message)
                        await clients[message["token"]].send(json.dumps(message))
                        #await clients[ddsagent].send(json.dumps(message))
                    except:
                        print("error: "+message)
            elif online_dict[client_id]==clientgroup[5]: #rtspproxy
                if message["token"] in clients:
                    try:
                        print(message)
                        await clients[message["token"]].send(json.dumps(message))
                    except:
                        print("error: "+message)
    finally:
        if client_id:
            del clients[client_id]
            del online_dict[client_id]
            print(f'Client {client_id} disconnected')
            newmessage={}
            newmessage["token"]=client_id
            newmessage["type"]="disconnect"
            if(ddsagent in clients):
                await clients[ddsagent].send(json.dumps(newmessage))
        

async def main():
    # Usage: ./server.py [[host:]port] [SSL certificate file]


    endpoint_or_port = sys.argv[1] if len(sys.argv) > 1 else "8010"
    ssl_cert = sys.argv[2] if len(sys.argv) > 2 else None

    endpoint = endpoint_or_port if ':' in endpoint_or_port else "0.0.0.0:" + endpoint_or_port

    if ssl_cert:
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ssl_context.load_cert_chain(ssl_cert)
    else:
        ssl_context = None

    print('Listening on {}'.format(endpoint))
    host, port = endpoint.rsplit(':', 1)

    server = await websockets.serve(handle_websocket, host, int(port), ssl=ssl_context)
    await server.wait_closed()
if __name__ == '__main__':
    asyncio.run(main())
