/// Connection object.
/// A simulated network connection with adjustable latency and packet loss.
/// This allows me to develop the networked physics code while having complete
/// control over the data transmission properties.
/// Effectively this object simulates a two way connection from client to server,
/// the client sends a stream of input to the server, while the server sends a stream
/// of corrections back to the client.

#include "Net.h"

class Connection
{
public:

    float latency;          ///< each way latency in seconds
    float packetLoss;       ///< percentage of packets lost
	net::Socket socket;
	net::Address clientAddress;
	bool firstReceive;
	float systemTime;

	float oldtime, temptime;

    Connection()
    {
        // defaults

        client = 0;
        server = 0;
        proxy = 0;

        latency = 0.0f;
        packetLoss = 0.0f;
        firstReceive = false;

        time = 0;

        #ifdef LOGGING
		//logfile = fopen("sync.log", "w");
		//logfile2 = fopen("insert_input_queue.log","w");
		//logfile3 = fopen("pop_input_queue.log","w");
		//logfile4 = fopen("insert_sync_queue.log","w");
		

        #endif
		logfile5 = fopen("clientEvent.log","w");
    }

    ~Connection()
    {
        #ifdef LOGGING
        if (logfile)
        {
			//fclose(logfile);
			//fclose(logfile2);
			//fclose(logfile3);
			//fclose(logfile4);
			
            logfile = 0;
			logfile2 = 0;
			logfile3 = 0;
			logfile4 = 0;
			logfile5 = 0;
        }
        #endif
		fclose(logfile5);
    }          

    void initialize(Client &client, Server &server, Proxy &proxy)
    {
		clientAddress = net::Address(127,0,0,1,30001);

		if ( !net::InitializeSockets() )
		{
			printf( "failed to initialize sockets\n" );
		}

		int port = 30000;

		printf( "creating socket on port %d\n", port );

		if ( !socket.Open( port ) )
		{
			printf( "failed to create socket!\n" );
		}
		
		this->client = &client;
        this->server = &server;
        this->proxy = &proxy;
    }

    void update(float absolutetime)
    {
		systemTime = absolutetime;

        // process event queues
		InputEvent *clientEvent = new InputEvent();
		InputEvent *buffer = new InputEvent();

		if(socket.Receive(clientAddress,buffer,sizeof(*clientEvent))){
			clientEvent->input = buffer->input;
			//clientEvent->importantMoves = buffer->importantMoves;
			clientEvent->time = buffer->time;
			clientEvent->isInputEvent = buffer->isInputEvent;
			clientEvent->clientTime = buffer->clientTime;
			clientEvent->clientstep = buffer->clientstep;
			clientEvent->serverTime = systemTime;
			clientEvent->serverstep = server->time;
			insert(clientToServer,clientEvent);
		}
		if(clientToServer.size()){
			if(systemTime - clientToServer.front()->serverTime >= latency){
				clientToServer.front()->serverTime = systemTime;
				clientToServer.front()->serverstep = server->time;
				fprintf(logfile5,"clientEvent, server time, %f, client Time, %f,server step, %d, client step, %d, step Time, %d, input jump, %d\n", clientToServer.front()->serverTime, clientToServer.front()->clientTime, ((InputEvent*)clientToServer.front())->serverstep, ((InputEvent*)clientToServer.front())->clientstep, ((InputEvent*)clientToServer.front())->time, ((InputEvent*)clientToServer.front())->input.jump);
				process(clientToServer);
			}
		}
    }

protected:

    /// input event recieved on server side

    void input(unsigned int t, Cube::Input &input, const std::vector<Move> &importantMoves, unsigned int deliveryTime)
    {
        // update server with input
		//fprintf(logfile3,"pop, client time, %d, event time, %d, delivery time, %d, input jump, %d\n", time, t, deliveryTime, input.jump);
        server->update(t, input, importantMoves);

        // send sync event back to client side
        SyncEvent *serverEvent = new SyncEvent();
        serverEvent->time = server->time;
        serverEvent->state = server->cube.state();
        serverEvent->input = input;
		serverEvent->isInputEvent = false;
		serverEvent->serverTime = systemTime;
		serverEvent->serverstep = server->time;
        //insert(serverToClient, event);
		if(!chance(packetLoss))
			socket.Send(clientAddress,serverEvent,sizeof(*serverEvent));

        #ifdef LOGGING
        if (logfile)
        {
            Vector position = serverEvent->state.position;
            Quaternion orientation = serverEvent->state.orientation;
            Cube::Input input = serverEvent->input;
            //fprintf(logfile, "%d, position, %f,%f,%f, orientation, %f,%f,%f,%f, input, %d,%d,%d,%d,%d\n", serverEvent->time, position.x, position.y, position.z, orientation.w, orientation.x, orientation.y, orientation.z, input.left, input.right, input.forward, input.back, input.jump);
        }
        #endif
    }

    /// synchronize event received on client side

    void synchronize(unsigned int t, const Cube::State &state, const Cube::Input &input, unsigned int deliveryTime)
    {
        //fprintf(logfile5,"pop, server time, %d, event time, %d, delivery time, %d, input jump, %d\n", time, t, deliveryTime, input.jump);
		client->synchronize(t, state, input);
        proxy->synchronize(t, state, input);
    }

private:

    struct Event
    {
        unsigned int deliveryTime;
		float clientTime;
		float serverTime;
		unsigned int clientstep;
		unsigned int serverstep;

		bool isInputEvent;
        virtual void execute(Connection &connection) = 0;
    };

    struct InputEvent : public Event
    {
        unsigned int time;
        Cube::Input input;
        std::vector<Move> importantMoves;
        void execute(Connection &connection)
        {
			connection.input(time, input, importantMoves, deliveryTime);
        }
    };

    struct SyncEvent : public Event
    {
        unsigned int time;
        Cube::State state;
        Cube::Input input;
        void execute(Connection &connection)
        {
            connection.synchronize(time, state, input, deliveryTime);
        }
    };

    typedef std::queue<Event*> EventQueue;

    EventQueue clientToServer;
    EventQueue serverToClient;

    /// insert an event into the queue

    void insert(EventQueue &queue, Event *event)
    {
        assert(event);
        //event->deliveryTime = time + (unsigned int) (latency/timestep);
		if(event->isInputEvent){
			//fprintf(logfile2,"insert, client time, %d, event time, %d,  delivery time, %d, input jump, %d\n", time, ((InputEvent*)event)->time, event->deliveryTime, ((InputEvent*)event)->input.jump);
		}else{
			//fprintf(logfile4,"insert, server time, %d, event time, %d,  delivery time, %d, input jump, %d\n", time, ((SyncEvent*)event)->time, event->deliveryTime, ((SyncEvent*)event)->input.jump);
		}
		queue.push(event);
    }

    /// process event queue and execute events ready for delivery

    void process(EventQueue &queue)
    {
        //while(queue.size()){
			Event *event = queue.front();

			event->execute(*this);
			queue.pop();
			delete event;
		//}
    }

    /// check if an event happens given a percentage frequency of occurance

    bool chance(float percent)
    {
        const float value = rand() / (float) RAND_MAX * 100;
        return value <= percent;
    }

    Client *client;
    Server *server;
    Proxy *proxy;

    FILE *logfile, *logfile2, *logfile3, *logfile4, *logfile5;

    unsigned int time;
};
