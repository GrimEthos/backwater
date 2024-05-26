# Architecture

## Network Connectivity

* client - connect and authenticate
* client - reconnect, reuse auth session
* client - request sessionId of server node
* client - send msg to session
* proxy - connect to session server, auth as proxy w/ nodeId
* proxy - request route to sessionId

msg: to_session_id, from_session_id, [len, status, bind, ?], data
	8 + 8 + 8 = 24 bytes min
