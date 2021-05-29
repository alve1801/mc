#define OLC_PGE_APPLICATION
#include <vector>
#include <math.h>
#include <vector>
using namespace std;
#include "olc.h"
#include "ppm.h"
#include "obj.h"
#define width 400
#define height 300
#define render_distance 16

short newmap[height*width],oldmap[height*width];char font[1<<14],str[256],sp;

/* XXX the following are for the parser, and operate on space-separated, null-terminated strings
XXX these were copied from nodes, please copy them back when implemented
bool match_string(char*src,char*dest,int n){} // returns true if the nth substring of src is dest
char*get_string(char*src,int n){} // returns the nth substring
int get_int(char*src,int n){} // returns the nth substring, evaluated as an integer
*/

World world;Entity player;

class Game : public olc::PixelGameEngine{public:
	Game(){sAppName="Game";}
	bool OnUserCreate()override{
		for(int x=0;x<height;x++)
		for(int y=0;y<width;y++)
		newmap[x*width+y]=0;
		render();
		return 1;
	}

	olc::Pixel sga(short color){
	int r = (color&0x0f00)>>4;
	int g = (color&0x00f0);
	int b = (color&0x000f)<<4;
	r=r|r>>4;g=g|g>>4;b=b|b>>4;
	return olc::Pixel(r,g,b);}

	void p(short c,int x,int y){Draw(y,x,sga(c));}

	void pc(char c,int sx,int sy,short fgc,short bgc){
		for(int x=0;x<8;x++)for(int y=0;y<8;y++)
		newmap[(sx*8+x)*width+sy*8+y]=(font[(unsigned char)c*8+x]&(char)(128>>y))?fgc:bgc;
	}

	void ps(const char*c,int sx,int sy,short fgc,short bgc){
		for(int i=0;c[i];i++){
			pc(c[i]==10?' ':c[i],sx,sy++,fgc,bgc);
			if(sy==width||c[i]==10){sy=0;sx++;} // XXX do we want to reset at initial xy?
		}
	}

	void draw_sprite(sprite s,int dx,int dy){ // XXX
		for(int x=0;x<s.x;x++)for(int y=0;y<s.y;y++)
		newmap[(dx+x)*width+dy+y]=s.data[x*s.y+y];
	}

	void refresh(){
		for(int x=0;x<height;x++)
		for(int y=0;y<width;y++)
		if(newmap[x*width+y]!=oldmap[x*width+y]){
			p(newmap[x*width+y],x,y);
			oldmap[x*width+y]=newmap[x*width+y];
		}
	}

	void render(){
		world.render(newmap,width,height,player,render_distance);
		newmap[height*width/2+width/2]=0x0fff;
		// interface overlay
	}

	bool OnUserUpdate(float fElapsedTime)override{
		int x=GetMouseY(),y=GetMouseX();
		if(GetKey(olc::ESCAPE).bPressed)return 0;

		float inc=.3;
		if(GetKey(olc::SHIFT).bHeld)inc=.02;
		if(GetKey(olc::TAB).bHeld)inc=1;

		player.dir=Q(0,0,0,player.dir.z); // remove all but the z component
		//player.dir=Q();

		if(GetKey(olc::SPACE).bHeld)
		if(world.getblock(qvi(player.loc-Q(0,0,0,endepth*2)))) // XXX
		player.dir.z=1;
		// also only if no z component present?

		if(GetKey(olc::SHIFT).bHeld)player.dir.z-=.5;

		Q roll  = Q(0,0,0,1).rot(player.rot.inv());
			roll  = Q(roll.w,0,0,roll.z).norm();
		Q pitch = Q(0,1,0,0).rot(player.rot.inv());
			pitch = Q(0,pitch.x,pitch.y,0).norm();
		Q yaw   = Q(0,0,1,0).rot(player.rot.inv());

		if(GetKey(olc::W).bHeld)player.dir+=pitch*inc;
		if(GetKey(olc::S).bHeld)player.dir-=pitch*inc;
		if(GetKey(olc::A).bHeld)player.dir-=yaw*inc;
		if(GetKey(olc::D).bHeld)player.dir+=yaw*inc;

		// XXX clip the following two
		if(GetKey(olc::UP).bHeld){
		yaw.w=inc/10;
		player.rot *= yaw.R();
		}

		if(GetKey(olc::DOWN).bHeld){
		yaw.w=-inc/10;
		player.rot *= yaw.R();
		}

		if(GetKey(olc::LEFT).bHeld){
		roll.w=-inc/10;
		player.rot *= roll.R();
		}

		if(GetKey(olc::RIGHT).bHeld){
		roll.w=inc/10;
		player.rot *= roll.R();
		}

		if(GetMouse(0).bPressed){printf("place\n");world.place(player,1,render_distance);}
		if(GetMouse(1).bPressed){printf("dig\n");world.dig(player,render_distance);} // XXX ignore return value for now

		player=world.collide(player);
		//player.loc+=player.dir;

		printf("(%i, %.3f s/f)\nloc dir rot:\n",world.chunks.size(),1/fElapsedTime);
		player.loc.print();
		player.dir.print();
		player.rot.print();

		// get server data somewhere here?
		// get user data
		// calculate response (physics etc)
		// chunk offloading etc

		render();
		refresh();
		putchar(10);
		return 1;
	}
};

int main(){
	FILE*f=fopen("font","r");for(int i=0;i<3200;i++)*(font+i)=getc(f);fclose(f);
	player.loc=Q(0,0,0,5);
	player.rot=Q(0,-1,0,0).norm();

	Game game;if(game.Construct(width,height,3,3))game.Start();return(0);
}
