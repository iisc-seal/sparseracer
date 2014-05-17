#include<iostream>

    #include<vector>
    using namespace std;
    
    
    int digits[10]={512,256,128,64,32,16,8,4,2,1};
    
    vector<long long> backarray(10);
    vector<long long> backpos(10);
    void backtrack(vector<int> &arr,vector<int> &value,pair<long long,long long>& solution,int i,long long sum=0,int ors=0)
    {
    	bool anychange=false;
    	int j=0;
    	if(i>1)
    		j=backpos[i-1]+1;
    	
    	for(;j<arr.size();j++)
    	{
    		
    		if(!(ors&arr[j]))
    		{
    			anychange=true;
    			backarray[i]=arr[j];
    			backpos[i]=j;
    			backtrack(arr,value,solution,i+1,sum+value[j],ors|arr[j]);
    		}
    	}
    	if(!anychange)
    	{
    
    		if(sum>solution.first)
    		{
    			solution.first=sum;
    			solution.second=i;
    		}
    		if(sum==solution.first)
    		{
    			if(i>solution.second)
    				solution.second=i;
    		}
    	}
    	
    }
    
    int main()
    {
    	int n;
    	backarray.resize(10);
    	backpos.resize(10);
    	while(cin>>n)
    	{
    		vector<int> arr(n),value(n);
    		for(int i=0;i<n;i++)
    		{
    			int a;
    			cin>>a;
    			value[i]=a;
    			while(a)
    			{
    				int r=a%10;
    				arr[i]|=digits[r];
    				a/=10;
    			}
    		}
    		pair<long long,long long> solution=make_pair(-1,-1);
    		backtrack(arr,value,solution,0);
    		cout<<solution.second<<endl;
    	}
    }

