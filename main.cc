
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

struct Rgb{ float r,g,b; };

Rgb screenbuffer[1920*1080];

int main(){
    SDL_Window *win = SDL_CreateWindow("Roaytroayzah", 0, 0, 640, 480, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);
    glClearColor(0,0,0,1);
    
    while(true){
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        int w,h; 
        SDL_GL_GetDrawableSize(win,&w,&h);
        
        glViewport(0, 0, w, h);

        // draw pixels
        for(int y=0; y<h; y++){
            for(int x=0; x<w; x++){
                screenbuffer[y*w+x] = (Rgb){(float)y/h,(float)x/w,0.f};
            }
        }

        // blit to screen
        glDrawPixels(w,h,GL_RGB,GL_FLOAT,&screenbuffer);
        SDL_GL_SwapWindow(win);

        // handle events
        SDL_Event e;
        SDL_WaitEvent(&e);
        do{ if(e.type==SDL_QUIT) 
                return 0; 
        }while(SDL_PollEvent(&e));
    }
}
