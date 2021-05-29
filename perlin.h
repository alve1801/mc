namespace perlin{
	// interleave high and low bits, then multiply by golden ratio for good spread
	unsigned char hash(int x){
		return 158*(x^(x>>4));
	}

	unsigned char hash(int x,int y){
		return hash(hash(x)^y);
	}

	unsigned char hash(int x,int y,int z){
		return hash(hash(x,y),z);
	}

	unsigned char _perl(int x,int s){
		if(s==0)return 0;
		return(
			hash(x/s)*(s-x%s) +
			hash(x/s+1)*(x%s)
		)/s;
	}

	unsigned char _perl(int x,int y,int s){
		x+=5;
		if(s==0)return 0;
		return(
			hash(x/s,y/s)*(s-x%s)*(s-y%s) +
			hash(x/s+1,y/s)*(x%s)*(s-y%s) +
			hash(x/s,y/s+1)*(s-x%s)*(y%s) +
			hash(x/s+1,y/s+1)*(x%s)*(y%s)
		)/(s*s);
	}

	unsigned char _perl(int x,int y,int z,int s){
		x+=5;y+=25;
		if(s==0)return 0;
		return(
			hash(x/s,y/s,z/s)*(s-x%s)*(s-y%s)*(s-z%s) +
			hash(x/s+1,y/s,z/s)*(x%s)*(s-y%s)*(s-z%s) +
			hash(x/s,y/s+1,z/s)*(s-x%s)*(y%s)*(s-z%s) +
			hash(x/s+1,y/s+1,z/s)*(x%s)*(y%s)*(s-z%s) +
			hash(x/s,y/s,z/s+1)*(s-x%s)*(s-y%s)*(z%s) +
			hash(x/s+1,y/s,z/s+1)*(x%s)*(s-y%s)*(z%s) +
			hash(x/s,y/s+1,z/s+1)*(s-x%s)*(y%s)*(z%s) +
			hash(x/s+1,y/s+1,z/s+1)*(x%s)*(y%s)*(z%s)
		)/(s*s*s);
	}

	unsigned char perl(int x,int y,int o){
		unsigned char ret=0;
		for(int i=1;i<o;i++)
			ret+=_perl(x,y,i)>>(o-i);
		return ret;
	}

	unsigned char perl(int x,int y,int z,int o){
		unsigned char ret=0;
		for(int i=1;i<o;i++)
			ret+=_perl(x,y,z,i)>>(o-i);
		return ret;
	}
};
