#include <unistd.h>
#include <thread>
#include "perlin.h"
#include "quat.h"
#define rad 360
#define fov 2 // basically a corrective factor
#define enwidth .7
#define enheight 1.7
#define endepth .1

int mod(int a,int b){a%=b;return(a>=0?a:b+a);}
float fmd(float a,float b=1){a=fmodf(a,b);return(a>0?a:b+a);} // motherfucker
int min(int a,int b){return(a>b?b:a);}
int max(int a,int b){return(a<b?b:a);}
int abs(int a){return a>0?a:-a;}
float sq(float a){return a*a;}

class vi{public:
	int x,y,z;
	vi(int x=0,int y=0,int z=0):x(x),y(y),z(z){}
	vi operator+(vi a){return vi(x+a.x,y+a.y,z+a.z);}
	vi operator-(vi a){return vi(x-a.x,y-a.y,z-a.z);}
	vi operator+(int a){return vi(x+a,y+a,z+a);}
	vi operator-(int a){return *this+(-a);}
	bool operator==(vi a){return x==a.x&&y==a.y&&z==a.z;}
	bool operator!=(vi a){return !(*this==a);}
	vi operator>>(int a){return vi((int)x>>a,(int)y>>a,(int)z>>a);} // XXX do we need this
	void print(){printf("%i %i %i\n",x,y,z);}
};

vi qvi(Q a){return vi((int)floorf(a.x),(int)floorf(a.y),(int)floorf(a.z));}

class UV{public:
	float u,v;
	UV(float u=0,float v=0):u(u),v(v){}
	// the following are mainly to simplify interpolation
	UV operator+(UV a){return UV(this->u+a.u,this->v+a.v);}
	UV operator-(UV a){return UV(this->u-a.u,this->v-a.v);}
	UV operator*(float a){return UV(this->u*a,this->v*a);}
	UV operator/(float a){return UV(this->u/a,this->v/a);}
};

class Tris{public:
	Q q[3];short col;float depth;
	short *texture,tsize;UV m[3]; // texture buffer (square)

	Tris(Q a=Q(),Q b=Q(),Q c=Q(),short col=0x0111):col(col){q[0]=a;q[1]=b;q[2]=c;tsize=0;};
};

// thanks to OneLoneCoder (aka javidx) for putting this code where i could shamelessly snatch it from
float interpol(Q pp,Q pn,Q ls,Q le){float pd=pn.dot(pp),ad=ls.dot(pn),bd=le.dot(pn);return (pd-ad)/(bd-ad);}
Q intersect(Q pp,Q pn,Q ls,Q le){return ls+(le-ls)*interpol(pp,pn,ls,le);}
int clip(Q pp, Q pn, Tris tin, Tris*tout1, Tris*tout2){
	// plane point, plane normal
	pn=pn.norm();
	// remap um,vm, account for order
	// ok this will be ugly but: we have to count up to three vertices, each w/ a mapping. cant we use a tris for that?

	tout1->texture=tout2->texture=tin.texture;
	tout1->tsize=tout2->tsize=tin.tsize;
	for(int i=0;i<3;i++)tout1->m[i]=tout2->m[i]=tin.m[i];

	Tris t;
	Q inside[3],outside[3];int ninside=0,noutside=2;pn=pn.norm();

	for(int i=0;i<3;i++)
	//((tin.q[i]-pp).norm().dot(pn)>0)?
	((tin.q[i]-pp).dot(pn)>0)?
		(t.q[ninside]=tin.q[i],t.m[ninside]=tin.m[i],ninside++):
		(t.q[noutside]=tin.q[i],t.m[noutside]=tin.m[i],noutside--);

	if(ninside==0)return 0; // invalid

	if(ninside==1){ // clip to 1
		*tout1=tin;
		tout1->q[0]=t.q[0];tout1->m[0]=t.m[0];

		tout1->q[1]=intersect(pp,pn,t.q[0],t.q[2]);
		tout1->m[1]=t.m[0]+(t.m[2]-t.m[0])*interpol(pp,pn,t.q[0],t.q[2]);

		tout1->q[2]=intersect(pp,pn,t.q[0],t.q[1]);
		tout1->m[2]=t.m[0]+(t.m[1]-t.m[0])*interpol(pp,pn,t.q[0],t.q[1]);
		return 1;
	}

	if(ninside==2){ // clip to 2
		*tout1=*tout2=tin;
		tout1->q[0]=t.q[0];tout1->m[0]=t.m[0];
		tout1->q[1]=t.q[1];tout1->m[1]=t.m[1];

		tout1->q[2]=intersect(pp,pn,t.q[0],t.q[2]);
		tout1->m[2]=t.m[0]+(t.m[2]-t.m[0])*interpol(pp,pn,t.q[0],t.q[2]);

		tout2->q[0]=t.q[1];tout2->m[0]=t.m[1];
		tout2->q[1]=tout1->q[2];tout2->m[1]=tout1->m[2];
		tout2->q[2]=intersect(pp,pn,t.q[1],t.q[2]);
		tout2->m[2]=t.m[1]+(t.m[2]-t.m[1])*interpol(pp,pn,t.q[1],t.q[2]);
		return 2;
	}

	if(ninside==3){*tout1=tin;return 1;} // valid
	return 0; // now shut up
}

class Chunk{public:
	vi loc;
	char data[16*16*16]; // indicates block types XXX should prolly use an enum...
	Chunk(){}
	Chunk(vi l):loc(l){
		printf("newchunk ");l.print();
		for(int x=0;x<16;x++)for(int y=0;y<16;y++)for(int z=0;z<16;z++)
		//data[(x*16+y)*16+z]=sq(l.z*16+z)>sq(l.x*16+x)+sq(l.y*16+y)?0:1; // cone

		data[(x*16+y)*16+z]=(l.z*16+z+5>(perlin::perl(l.x*16+x,l.y*16+y,8)>>6))?0:1;
		//data[(x*16+y)*16+z]=(l.z*16+z>(perlin::perl(l.x*16+x,l.y*16+y,8)>>6))?0:1; // ol' reliable
		//data[(x*16+y)*16+z]=(l.z*16+z>(perlin::perl(l.x*16+x,l.y*16+y,3)>>4))?0:1; // noisy
		//data[(x*16+y)*16+z]=l.z*16+z+l.x+l.y>0?0:1; // steps
		//data[(x*16+y)*16+z]=l.z*16+z>0?0:1; // superflat
	}

	Chunk(FILE*f,vi l){
		printf("loadchunk\n");
		// XXX try to load from file
		//Chunk(l); XXX gdi
	}

	char getblock(vi l){return data[(mod(l.x,16)*16+mod(l.y,16))*16+mod(l.z,16)];}
};

class Entity{public:
	Q loc,rot,dir;
	// XXX inventory
	Entity(Q loc=Q()):loc(loc),dir(Q()),rot(Q().R()){}
};

char ind(char*data,int s,vi loc){ // for indexing an array of data (easier to debug here)
	s*=2;
	// XXX check oob?
	return data[(loc.x*s+loc.y)*s+loc.z];
}

class World{public:
	vector<Entity>entities;
	vector<Chunk>chunks;
	FILE*f; // savefile
	World(){}
	World(char*filename){} // XXX

	char getblock(vi l){
		for(Chunk c:chunks)
			if(c.loc==l>>4)
				return c.getblock(l);
		chunks.push_back(Chunk(l>>4));
		return chunks[chunks.size()-1].getblock(l);
	}

	char*getrange(char*data,vi loc,int r){
		// like the above, but returns a whole bunch of them (we can optimize it some (i hope))
		// also this definitely generates chunks
		// XXX cant we make it allocate its own buffer?
		for(int x=0;x<r*2;x++)for(int y=0;y<r*2;y++)for(int z=0;z<r*2;z++)
		data[(x*r*2+y)*r*2+z]=getblock(loc+vi(x-r,y-r,z-r));
		return data;
	}

	Entity collide(Entity p){
		Entity res=p;
		if(!getblock(qvi(res.loc-Q(0,0,endepth*2)))){
			printf("fall\n");
			res.dir.z-=.2;
		}

		res.loc+=res.dir;

		// below
		if(getblock(qvi(res.loc))){
			printf("down\n");
			res.loc.z=res.loc.z+1-fmd(res.loc.z)+endepth;
			res.dir.z=0;
		}

		// X (red)
		if(getblock(qvi(res.loc+Q( enwidth/2,0,0)))){ // front - jumps back
			printf("front\n");
			res.loc.x=res.loc.x  -fmd(res.loc.x+enwidth/2);
		}
		if(getblock(qvi(res.loc+Q(-enwidth/2,0,0)))){ // back - works
			printf("back\n");
			res.loc.x=res.loc.x+1-fmd(res.loc.x-enwidth/2);
		}

		// Y (green)
		if(getblock(qvi(res.loc+Q(0, enwidth/2,0)))){ // left - jumps back
			res.loc.y=res.loc.y  -fmd(res.loc.y+enwidth/2);
		}
		if(getblock(qvi(res.loc+Q(0,-enwidth/2,0)))){ // right - works
			res.loc.y=res.loc.y+1-fmd(res.loc.y-enwidth/2);
		}

		return res;
	}

	void place(Entity p,char blocktype,int rend){
		// XXX casts a ray to figure out what block the entity is looking at, sets it to blocktype
	}

	char dig(Entity p,int rend){
		// XXX the opposite of place, it deletes the block and returns its type
		// XXX also this prolly shouldnt use getrange... itll be slower, but we need global index
		// XXX speaking of, we also have to save the indexes to the blocks collided on every axis...

		char*worlddata=(char*)malloc(rend*rend*rend*8);
		worlddata=getrange(worlddata,qvi(p.loc),rend);
		Q delta,current,corr=corr=p.loc.round()-Q(rend,rend,rend);
		Q rot=Q(0,1,0).rot(p.rot.inv()) *-1;
		Q depth=Q(1000,1000,1000);
		vi t;

		// X
		//delta=rot/fabs(rot.x);
		delta=rot*((rot.x>0?1:-1)/rot.x);
		current=p.loc+delta*fmod(p.loc.x,1)*(delta.x>0?-1:1);
		while((p.loc-current).sqabs()<rend*rend){
		t=qvi(current-corr);
		if(delta.x<0)t.x--;
		if(ind(worlddata,rend,t)){
		depth.x=(p.loc-current).abs();
		break;
		}
		current+=delta;
		}


		// Y
		delta=rot*((rot.y>0?1:-1)/rot.y);
		current=p.loc+delta*fmod(p.loc.y,1)*(delta.y>0?-1:1);
		while((p.loc-current).sqabs()<rend*rend){
		t=qvi(current-corr);
		if(delta.y<0)t.y--;
		if(ind(worlddata,rend,t)){
		depth.y=(p.loc-current).abs();
		break;
		}
		current+=delta;
		}


		// Z
		delta=rot*((rot.z>0?1:-1)/rot.z);
		current=p.loc+delta*fmod(p.loc.z,1)*(delta.z>0?-1:1);
		while((p.loc-current).sqabs()<rend*rend){
		t=qvi(current-corr);
		if(delta.z<0)t.z--;
		if(ind(worlddata,rend,t)){
		depth.z=(p.loc-current).abs();
		break;
		}
		current+=delta;
		}

		/*
		if(depth.x==1000&&depth.y==1000&&depth.z==1000)canvas[sx*w+sy]=0;
		else if(depth.x<depth.y&&depth.x<depth.z)canvas[sx*w+sy]=0x0f22;
		else if(depth.y<depth.z)canvas[sx*w+sy]=0x02f2;
		else canvas[sx*w+sy]=0x022f;
		*/

	}





	// ~~~  graphics  ~~~


	void p(short*canvas,int w,int h,int x,int y,short c){
		if(0<x&&x<w&&0<y&&y<h)
			canvas[y*w+x]=c;
	}

	void edge(short*canvas,int w,int h,Q a,Q b,short c=0x0f11){
		int x,y,i;
		float d=abs(abs(a.x-b.x)>abs(a.y-b.y)?a.x-b.x:a.y-b.y);
		for(i=0;i<(int)d;i++){
			x=(int)((a.x*i+b.x*(d-i))/d);
			y=(int)((a.y*i+b.y*(d-i))/d);
			p(canvas,w,h,x,y,c);
		}
	}

	short*render(short*canvas,int w,int h,Entity rin,int rend){
		char*worlddata=(char*)malloc(rend*rend*rend*8);
		worlddata=getrange(worlddata,qvi(rin.loc),rend);
		for(int i=0;i<h*w;i++)canvas[i]=0; // poor man's memset

		int col,x1,x2,t;
		vector<Tris>finals,temp;Tris clipped[2];
		//Q pr=rin.loc.round()+Q(rend,rend,rend)+Q(0,.5,.5,-enheight);
		//Q pr=rin.loc.round()+Q(rend,rend,rend)+Q(0,0,0,enheight);
		Q pr=Q(fmd(rin.loc.x),fmd(rin.loc.y),fmd(rin.loc.z))+Q(rend,rend,rend)+Q(0,0,0,enheight);

		for(int x=1;x<rend*2-1;x++)
		for(int y=1;y<rend*2-1;y++)
		for(int z=1;z<rend*2-1;z++)
		if(ind(worlddata,rend,vi(x,y,z))){

			if(!ind(worlddata,rend,vi(x,y,z-1))){
				// below
				finals.push_back(Tris(
					Q(x,y,z)-pr,
					Q(x,y+1,z)-pr,
					Q(x+1,y,z)-pr,
					0x00f
				));
				finals.push_back(Tris(
					Q(x+1,y+1,z)-pr,
					Q(x+1,y,z)-pr,
					Q(x,y+1,z)-pr,
					0x00f
				));
			}


			if(!ind(worlddata,rend,vi(x,y,z+1))){
				// above
				finals.push_back(Tris(
					Q(x,y,z+1)-pr,
					Q(x+1,y,z+1)-pr,
					Q(x,y+1,z+1)-pr,
					0x22f
				));
				finals.push_back(Tris(
					Q(x+1,y+1,z+1)-pr,
					Q(x,y+1,z+1)-pr,
					Q(x+1,y,z+1)-pr,
					0x22f
				));
			}


			if(!ind(worlddata,rend,vi(x,y-1,z))){
				// left
				finals.push_back(Tris(
					Q(x,y,z)-pr,
					Q(x+1,y,z)-pr,
					Q(x,y,z+1)-pr,
					0x0f0
				));
				finals.push_back(Tris(
					Q(x+1,y,z+1)-pr,
					Q(x,y,z+1)-pr,
					Q(x+1,y,z)-pr,
					0x0f0
				));
			}


			if(!ind(worlddata,rend,vi(x,y+1,z))){
				// right
				finals.push_back(Tris(
					Q(x,y+1,z)-pr,
					Q(x,y+1,z+1)-pr,
					Q(x+1,y+1,z)-pr,
					0x2f2
				));
				finals.push_back(Tris(
					Q(x+1,y+1,z+1)-pr,
					Q(x+1,y+1,z)-pr,
					Q(x,y+1,z+1)-pr,
					0x2f2
				));
			}


			if(!ind(worlddata,rend,vi(x-1,y,z))){
				// back
				finals.push_back(Tris(
					Q(x,y,z)-pr,
					Q(x,y,z+1)-pr,
					Q(x,y+1,z)-pr,
					0xf00
				));
				finals.push_back(Tris(
					Q(x,y+1,z+1)-pr,
					Q(x,y+1,z)-pr,
					Q(x,y,z+1)-pr,
					0xf00
				));
			}


			if(!ind(worlddata,rend,vi(x+1,y,z))){
				// front
				finals.push_back(Tris(
					Q(x+1,y,z)-pr,
					Q(x+1,y+1,z)-pr,
					Q(x+1,y,z+1)-pr,
					0xf22
				));
				finals.push_back(Tris(
					Q(x+1,y+1,z+1)-pr,
					Q(x+1,y,z+1)-pr,
					Q(x+1,y+1,z)-pr,
					0xf22
				));
			}
		}

		printf("%i faces\n",finals.size());

		temp=finals;finals.clear();
		for(Tris t:temp){
			t.q[0]=t.q[0].rot(rin.rot);
			t.q[1]=t.q[1].rot(rin.rot);
			t.q[2]=t.q[2].rot(rin.rot);

			Q norm=((t.q[1]-t.q[0])*(t.q[2]-t.q[0])).norm(); // XXX it doesnt actually need to be normalized
			Q loc=(t.q[0]+t.q[1]+t.q[2]); // /3 but we dont care
			if(norm.dot(loc)>0){ // XXX this does not do what i think it does
				//if(tf.col==0x0fff || drawcolors)tf.col=(char)(cdir*0xf)*0x0111; // plz shade?
				t.depth=loc.sqabs();
				finals.push_back(t);
			}
		}

		// clipping preserves order, and theres fewer tris here
		sort(finals.begin(),finals.end(),[](Tris&t1,Tris&t2){return t1.depth>t2.depth;});

		// XXX the following is very repetitive, can we simplify it?

		// front plane
		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,.1,0,0),Q(0,1,0,0),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		// frustrum clipping
		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,0,0,0),Q(0,1,.5,0),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,0,0,0),Q(0,1,-.5,0),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,0,0,0),Q(0,1,0,.5),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,0,0,0),Q(0,1,0,-.5),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		// back plane - use rend*2?
		temp=finals;finals.clear();
		for(Tris tf:temp){
			int nclip = clip(Q(0,rend,0,0),Q(0,-1,0,0),tf,clipped,clipped+1);
			for(int i=0;i<nclip;i++)finals.push_back(clipped[i]);
		}

		// flatten
		temp=finals;finals.clear();
		for(Tris tf:temp){
			for(int i=0;i<3;i++)
			tf.q[i]=tf.q[i].flat(fov,h,w);
			finals.push_back(tf);
		}

		//if(!wireframe)
		for(Tris f:finals){
			col=f.col;

			int
			ax=(int)f.q[0].x, ay=(int)f.q[0].y,
			bx=(int)f.q[1].x, by=(int)f.q[1].y,
			cx=(int)f.q[2].x, cy=(int)f.q[2].y;
			UV am=f.m[0],bm=f.m[1],cm=f.m[2],tm;
			float ad=f.q[0].z,bd=f.q[1].z,cd=f.q[2].z;

			// XXX vertex depth for texture interpolation
			// store uv/z instead of uv
			// interpolate uz vz z
			// use uv*z for coordinates

			// * XXX separate x&y, handle separately?
			// idk how but we must put the coords in the right order
			// i think the problem with this here is that it does not retain their chirality, which we need to make sure the texture is the right way around
			// now how tf do i fix that
			if(ay>cy){t=ax;ax=cx;cx=t;t=ay;ay=cy;cy=t;tm=am;am=cm;cm=tm;}
			if(ay>by){t=ax;ax=bx;bx=t;t=ay;ay=by;by=t;tm=am;am=bm;bm=tm;}
			if(by>cy){t=bx;bx=cx;cx=t;t=by;by=cy;cy=t;tm=bm;bm=cm;cm=tm;}
			// */

			// XXX reorder vertices

			/*
			if(ay>cy){t=ax;ax=cx;cx=t;t=ay;ay=cy;cy=t;}
			if(ay>by){t=ax;ax=bx;bx=t;t=ay;ay=by;by=t;}
			if(by>cy){t=bx;bx=cx;cx=t;t=by;by=cy;cy=t;}
			// */

			if(ay==cy){ // or just ignore it?
				if(ax>cx){t=ax;ax=cx;cx=t;}
				if(ax>bx){t=ax;ax=bx;bx=t;}
				if(bx>cx){t=bx;bx=cx;cx=t;}
				for(int x=ax;x<cx;x++)p(canvas,w,h,x,ay,col);
				continue;
			}

			if(f.tsize!=0){ // textured
				float u,v,u1,u2,v1,v2;
				//float d,d1,d2;

				// make sure uv coords match vertices first before fixing this

				if(ay!=by)
					for(int y=ay;y<by;y++){
						x1 = ax+(y-ay)*(ax-bx)/(ay-by);
						x2 = ax+(y-ay)*(ax-cx)/(ay-cy);
						//d1 = ad+(ad-bd)*(y-ay)/(by-ay);
						//d2 = ad+(ad-cd)*(y-ay)/(cy-ay);

						u1 = am.u+(bm.u-am.u)*(y-ay)/(by-ay);
						u2 = am.u+(cm.u-am.u)*(y-ay)/(cy-ay);
						//if(cx<bx){u=u1;u1=u2;u2=u;}

						// bx>cx then it errs
						v1 = am.v+(bm.v-am.v)*(y-ay)/(by-ay);
						v2 = am.v+(cm.v-am.v)*(y-ay)/(cy-ay);

						if(x1>x2){t=x1;x1=x2;x2=t;}
						for(int x=x1;x<x2;x++){
							//d=d1+(d2-d1)*(x-x1)/(x2-x1);
							v = v1+(v2-v1)*(x-x1)/(x2-x1);
							u = u1+(u2-u1)*(x-x1)/(x2-x1);
							p(canvas,w,h,x,y,f.texture[mod((int)u,f.tsize)*f.tsize+mod((int)v,f.tsize)]);
						}
					}

				if(by!=cy)
					for(int y=by;y<cy;y++){
						x1 = ax+(y-ay)*(ax-cx)/(ay-cy);
						x2 = bx+(y-by)*(bx-cx)/(by-cy);

						u1 = bm.u+(cm.u-bm.u)*(y-by)/(cy-by);
						u2 = am.u+(cm.u-am.u)*(y-ay)/(cy-ay);

						v1 = bm.v+(cm.v-bm.v)*(y-by)/(cy-by);
						v2 = am.v+(cm.v-am.v)*(y-ay)/(cy-ay);
						//if(cx<bx){v=v1;v1=v2;v2=v;}

						if(x1>x2){t=x1;x1=x2;x2=t;}
						for(int x=x1;x<x2;x++){
							u = u1+(u2-u1)*(x-x1)/(x2-x1);
							v = v1+(v2-v1)*(x-x1)/(x2-x1);
							p(canvas,w,h,x,y,f.texture[mod((int)u,f.tsize)*f.tsize+mod((int)v,f.tsize)]);
						}
					}

			}else{ // flat shading
				if(ay!=by)
					for(int y=ay;y<by;y++){
						x1 = ax+(y-ay)*(ax-bx)/(ay-by);
						x2 = ax+(y-ay)*(ax-cx)/(ay-cy);
						if(x1>x2){t=x1;x1=x2;x2=t;}
						for(int x=x1;x<x2;x++)
							//if(x%2==y%2 && y%2==0)buf[y*w+x]=0x0fff^col;else // flat shading
							p(canvas,w,h,x,y,col);
					}

				if(by!=cy)
					for(int y=by;y<cy;y++){
						x1 = ax+(y-ay)*(ax-cx)/(ay-cy);
						x2 = bx+(y-by)*(bx-cx)/(by-cy);
						if(x1>x2){t=x1;x1=x2;x2=t;}
						for(int x=x1;x<x2;x++)
							p(canvas,w,h,x,y,col);
					}
				}

			edge(canvas,w,h,f.q[0],f.q[1]);edge(canvas,w,h,f.q[1],f.q[2]);edge(canvas,w,h,f.q[2],f.q[0]);

		}

		/*
		if(drawwisps){
			for(P point:wisps){ // debug
				qt=(point.loc-loc).rot(rot).flat(view.fov,view.h,view.w);
				x1=(int)qt.x;x2=(int)qt.y;
				if(qt.z>0)p(view,x1,x2,point.col);
			}

			for(int x=0;x<wisps.size();x++)
			for(int y=x+1;y<wisps.size();y++)
				if(wisps[x].col==wisps[y].col){
				Q a=(wisps[x].loc-loc).rot(rot).flat(view.fov,view.h,view.w);
				Q b=(wisps[y].loc-loc).rot(rot).flat(view.fov,view.h,view.w);
				if(a.z>0 && b.z>0)
				edge(view,a,b,wisps[x].col);
			}
		}
		*/

		return canvas;
	}
};
