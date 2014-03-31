This is a http server module for Tundra that implements the SceneAPI part of the
Synchronization Generic Enabler in the FI-WARE project.

It utilizes a forked copy of the websocketpp library from 
https://github.com/jdeng/websocketpp which implements reading of PUT/POST request
body data. This is different than the official websocketpp 0.3.0 library used in 
Tundra dependencies for the WebSocketServerModule.

To enable this module, the command line parameter --httpport needs to be given. 
For example --httpport 80

The module will react to http requests that begin with the path /scene or
/entities. Other requests will be emitted as a signal so that other parties can
handle them.
