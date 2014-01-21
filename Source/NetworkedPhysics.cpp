// Zen of Networked Physics
// Copyright (c) Glenn Fiedler 2004
// http://www.gaffer.org/articles

#define LOGGING
#define DEVELOPMENT

#pragma warning( disable : 4127 )  // conditional expression is constant
#pragma warning( disable : 4100 )  // unreferenced formal parameter
#pragma warning( disable : 4702 )  // unreachable code
#pragma warning( disable : 4244 )  // double to float
#pragma warning( disable : 4996 )  // stupid deprecated warnings

const float timestep = 0.01f;

#include "Mathematics.h"
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"

using namespace Mathematics;

// platform specific functions

bool openDisplay(const char title[], int width, int height, bool fullscreen = false);
void updateDisplay();
void closeDisplay();
void drawText(float x, float y, const char text[], Vector color = Vector(1,1,1), float alpha = 1);
float time();

enum Key 
{ 
    Left,
    Right,
    Up,
    Down,
    Space,
    Enter,
    Control,
    Esc,
    PageUp,
    PageDown,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
	STOP, //autokey
};

void onKeyUp(Key key);
void onKeyDown(Key key);
void onQuit();

#include <vector>
#include <queue>
#include "Apple.h"
#include "Windows.h"

// platform independent

#include "FreeType.h"
#include "Font.h"

Font font;

#include "Plane.h"
#include "OpenGL.h"
#include "Cube.h"
#include "Scene.h"
#include "Move.h"
#include "History.h"
#include "Client.h"
#include "Server.h"
#include "Proxy.h"
#include "Text.h"
#include "Page.h"
#include "Panel.h"
#include "View.h"
#include "Connection.h"

Client client;
Server server;
Proxy proxy;

View view;

Connection connection;

// user input handlers

#include "Input.h"

bool quit = false;

Input input;

void onKeyDown(Key key) { input.onKeyDown(key); }
void onKeyUp(Key key) { input.onKeyUp(key); }
void onQuit()
{
	quit = true;
}


// options manager

#include "Options.h"

Options options;


int main()
{
	const int width = 800;
	const int height = 600;

#ifdef DEVELOPMENT
    if (!openDisplay("Zen of Networked Physics", width, height, false))
#else
    if (!openDisplay("Zen of Networked Physics", width, height, true))
#endif
        return 1;

	initializeOpenGL();
	
    view.initialize(client, server, proxy);
    connection.initialize(client, server, proxy);

	font.initialize();

    input.listener = &options;

	unsigned int t = 0;
    float accumulator = 0.0f;

    float absoluteTime = 0.0f;
    
	float oldTime = 0; //autokey timer

	char *filename = "time.log"; //time log
	char *filename2 = "time2.log";
	FILE *logfile = fopen(filename,"w"); //time log
	FILE *logfile2 = fopen(filename2,"w");












    while (!quit) 
	{			
        // update absolute time

        float newTime = time();
        float deltaTime = newTime - absoluteTime;
		

        if (deltaTime<=0.0f)
            continue;

        options.update(absoluteTime, deltaTime);

        absoluteTime = newTime;

		accumulator += deltaTime;

        

		//autokey
		if(newTime >= 5){
			onQuit();
		}else if(newTime >= 0){
			//fprintf(logfile2,"newtime, %f, oldtime, %f\n",newTime,oldTime);
			if((newTime - oldTime)<0.02){
				input.onKeyDown(Space);
				//input.onKeyDown(Right);
			}else if((newTime - oldTime)<0.5){
				input.onKeyUp(Space);
			}else{
				oldTime = newTime;
			}
			//else if((newTime - oldTime)<1){
			//	input.onKeyUp(Space);
			//}else if((newTime - oldTime)<2){
			//	input.onKeyUp(Right);
			//}else if((newTime - oldTime)<2.03){
			//	input.onKeyDown(Space);
			//	input.onKeyDown(Left);
			//}else if((newTime - oldTime)<3){
			//	input.onKeyUp(Space);
			//}else if((newTime - oldTime)<4){
			//	input.onKeyUp(Left);
			//}else{
			//	oldTime = newTime;
			//}
		}

        // update discrete time
		while (accumulator>=timestep)
        {
            
			fprintf(logfile,"newTime, %f, absoluteTime, %f, accumulator, %f, discrete time, %d, input jump, %d\n", newTime, absoluteTime, accumulator, t, input.space()); //time log
			
			// update input

            input.update(t);

            client.input.left = input.left();
            client.input.right = input.right();
            client.input.forward = input.up();
            client.input.back = input.down();
            client.input.jump = input.space();

            // update connection

            connection.update(t);

            // update scenes

            client.update(t);
            proxy.update(t);
            view.update(t);

            // advance discrete time

            accumulator -= timestep;
            t++;
			fprintf(logfile,"newTime, %f, absoluteTime, %f, accumulator, %f, discrete time, %d, input jump, %d\n", newTime, absoluteTime, accumulator, t, input.space()); //time log
        }

		

        // render view

        view.render(accumulator/timestep);

        // update display

        updateDisplay();
	}
	
	fclose(logfile); //time log
	fclose(logfile2);

	closeDisplay();
	
	return 0;
}
