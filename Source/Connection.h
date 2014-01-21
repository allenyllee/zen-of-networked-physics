/// Connection object.
/// A simulated network connection with adjustable latency and packet loss.
/// This allows me to develop the networked physics code while having complete
/// control over the data transmission properties.
/// Effectively this object simulates a two way connection from client to server,
/// the client sends a stream of input to the server, while the server sends a stream
/// of corrections back to the client.

class Connection
{
public:

    float latency;          ///< each way latency in seconds
    float packetLoss;       ///< percentage of packets lost

    Connection()
    {
        // defaults

        client = 0;
        server = 0;
        proxy = 0;

        latency = 0.0f;
        packetLoss = 0.0f;
        
        time = 0;

        #ifdef LOGGING
        logfile = fopen("sync.log", "w");
		logfile2 = fopen("insert_input_queue.log","w");
		logfile3 = fopen("pop_input_queue.log","w");
		logfile4 = fopen("insert_sync_queue.log","w");
		logfile5 = fopen("pop_sync_queue.log","w");

        #endif
    }

    ~Connection()
    {
        #ifdef LOGGING
        if (logfile)
        {
            fclose(logfile);
			fclose(logfile2);
			fclose(logfile3);
			fclose(logfile4);
			fclose(logfile5);
            logfile = 0;
			logfile2 = 0;
			logfile3 = 0;
			logfile4 = 0;
			logfile5 = 0;
        }
        #endif
    }          

    void initialize(Client &client, Server &server, Proxy &proxy)
    {
        this->client = &client;
        this->server = &server;
        this->proxy = &proxy;
    }

    void update(unsigned int t)
    {
        // update time

        time = t;

        // process event queues

        process(clientToServer);
        process(serverToClient);

        // send input event to server

        InputEvent *event = new InputEvent();

        event->time = client->time;
        event->input = client->input;
        client->history.importantMoveArray(event->importantMoves);
		event->isInputEvent = true; //is input event?
        insert(clientToServer, event);

        // step ahead

        time ++;
    }

protected:

    /// input event recieved on server side

    void input(unsigned int t, Cube::Input &input, const std::vector<Move> &importantMoves, unsigned int deliveryTime)
    {
        // update server with input
		fprintf(logfile3,"pop, client time, %d, event time, %d, delivery time, %d, input jump, %d\n", time, t, deliveryTime, input.jump);
        server->update(t, input, importantMoves);

        // send sync event back to client side

        SyncEvent *event = new SyncEvent();
        event->time = server->time;
        event->state = server->cube.state();
        event->input = input;
		event->isInputEvent = false;
        insert(serverToClient, event);

        #ifdef LOGGING
        if (logfile)
        {
            Vector position = event->state.position;
            Quaternion orientation = event->state.orientation;
            Cube::Input input = event->input;
            fprintf(logfile, "%d, position, %f,%f,%f, orientation, %f,%f,%f,%f, input, %d,%d,%d,%d,%d\n", event->time, position.x, position.y, position.z, orientation.w, orientation.x, orientation.y, orientation.z, input.left, input.right, input.forward, input.back, input.jump);
        }
        #endif
    }

    /// synchronize event received on client side

    void synchronize(unsigned int t, const Cube::State &state, const Cube::Input &input, unsigned int deliveryTime)
    {
        fprintf(logfile5,"pop, server time, %d, event time, %d, delivery time, %d, input jump, %d\n", time, t, deliveryTime, input.jump);
		client->synchronize(t, state, input);
        proxy->synchronize(t, state, input);
    }

private:

    struct Event
    {
        unsigned int deliveryTime;
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
        event->deliveryTime = time + (unsigned int) (latency/timestep);
		if(event->isInputEvent){
			fprintf(logfile2,"insert, client time, %d, event time, %d,  delivery time, %d, input jump, %d\n", time, ((InputEvent*)event)->time, event->deliveryTime, ((InputEvent*)event)->input.jump);
		}else{
			fprintf(logfile4,"insert, server time, %d, event time, %d,  delivery time, %d, input jump, %d\n", time, ((SyncEvent*)event)->time, event->deliveryTime, ((SyncEvent*)event)->input.jump);
		}
		queue.push(event);
    }

    /// process event queue and execute events ready for delivery

    void process(EventQueue &queue)
    {
        while (queue.size())
        {
            Event *event = queue.front();

            if (event->deliveryTime<=time)
            {
                if (!chance(packetLoss))
                    event->execute(*this);

                queue.pop();

                delete event;
            }
            else
                break;
        }
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
