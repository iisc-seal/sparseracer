#include<iostream>

    #define L(i,s,f) for(i=s;i<f;i++)
    #define max(a,b) (a)>(b)?(a):(b)
    
    using namespace std;
    
    int DP[105][106],K[105],C[105],P[105],T[105];
    
    int main()
    {
    	int Tc,N,W,i,j,d;
    	cin>>Tc;
    	while(Tc--)
    	{
    		cin>>N>>W;
    		C[0]=P[0]=T[0]=0;
    		
    		for(int i=1; i<N+1; i++)
    		{
    			cin>>C[i]>>P[i]>>T[i];
    			K[i]=P[i]*C[i];
    		}
    		
    		for(int i=0; i<W+1; i++)
    			DP[0][i]=0;
    		
    		L(i,1,N+1)
    		{
    			L(j,0,W+1)
    			{
    				d=j-T[i];
    				if(d>=0)
    					DP[i][j]=max(DP[i-1][j],(K[i]+DP[i-1][d]));
    				else
    					DP[i][j]=DP[i-1][j];
    			}
    		}
    		cout<<DP[N][W]<<endl;
    	}
    	return 0;
    }

