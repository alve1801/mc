class Q{public:float w,x,y,z;
	Q():w(0),x(0),y(0),z(0){};Q(float x,float y,float z):w(0),x(x),y(y),z(z){};
	Q(float w,float x,float y,float z):w(w),x(x),y(y),z(z){};
	void print(){printf("% .3f % .3f % .3f % .3f\n",this->w,this->x,this->y,this->z);}
	float sqabs(){return(this->w*this->w+this->x*this->x+this->y*this->y+this->z*this->z);}
	float abs(){return sqrt(sqabs());}
	Q norm(){float a=this->abs();if(a==0)return Q();return *this/a;}
	Q inv(){return Q(this->w,-this->x,-this->y,-this->z);}
	float dot(Q a){return (this->w*a.w +this->x*a.x +this->y*a.y +this->z*a.z);}
	Q operator+(Q a){return Q(this->w+a.w,this->x+a.x,this->y+a.y,this->z+a.z);}
	Q operator+=(Q a){return(*this=*this+a);}
	Q operator-(Q a){return Q(this->w-a.w,this->x-a.x,this->y-a.y,this->z-a.z);}
	Q operator-=(Q a){return(*this=*this-a);}
	Q operator*(Q a){/*BxA*/Q b=*this;return Q(a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w);}
	Q operator*(float a){return Q(this->w*a,this->x*a,this->y*a,this->z*a);}
	Q operator*=(Q a){return(*this=*this*a);}
	Q operator*=(float a){return(*this=*this*a);}
	Q operator/(float a){return Q(this->w/a,this->x/a,this->y/a,this->z/a);}
	Q operator/=(float a){return(*this=*this/a);}
	bool operator==(Q a){return this->w==a.w&&this->x==a.x&&this->y==a.y&&this->z==a.z;}
	bool operator!=(Q a){return !(*this==a);}
	Q R(){/*units*/float s=sinf(this->w*M_PI);return Q(cosf(this->w*M_PI),s*this->x,s*this->y,s*this->z).norm();}
	Q rot(Q q){/*q=q.norm();*/return q*(*this)*q.inv();} // aka conj qaq'
	Q flat(float fov,int h,int w){if(this->x==0)return Q();return Q(0,this->y/this->x/fov*w/2+w/2,this->z/this->x/fov*w/2+h/2,this->x);}
	//Q round(){return Q((int)w,(int)x,(int)y,(int)z);}
	Q round(){return Q(fmod(x,1),fmod(y,1),fmod(z,1));}
};
