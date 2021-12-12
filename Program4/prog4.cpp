#include <iostream>
#include <vector>
#include <deque>

#include "prog4_lru.h"
#include "prog4_2nd.h"
//
//	Uncomment the following lines to test your program
//  #include "prog4_lru.h"
//  #include "prog4_2nd.h"
//  #include "prog4_myown.h"

using namespace std;

/*
	Data Structure that is used for page replacement algorithm

	For FIFO, this is just a FIFO queue
*/

class PRDS_FIFO
{
public:
	PRDS_FIFO(int x){}; // x denotes the number of size of the page vector (max # of pages in main memory)

	void push(int x) { q.push_back(x); };
	int pop()
	{
		int x = q.front();
		q.pop_front();
		return (x);
	};
	int size() { return q.size(); };

	deque<int> q;
};

/*
	Class you need to implement
	(No need to uncomment these lines



  class PRDS_LRU {}; // Data Structure for the LRU
  class PRDS_2nd {}; // Data Structure for the 2nd chance algorithm
  class PRDS_MyOwn {}; // Data Structure for the your own algorithm
*/

/*

	Page Replacement algorithm:

	Parameter:	
		pages: a vector storing the set of pages that is curently in memory
		nextpage: the next page to be accesses
		p : a pointer to the data structure that is used for the page replacement

	Output:
		-1 : if no page replacement is needed
		any other number: the index in the pages vector that is to be replaced

*/

int Page_Replacement(vector<int> &pages, int nextpage, PRDS_FIFO *p)
{
	bool found = false;
	int i;

	/*
      Check if nextpage is in the pages array, if so return -1
   */
	for (i = 0; i < pages.size(); i++)
	{
		if (pages[i] == nextpage)
		{
			found = true;
			return -1;
		}
	}

	if (not found)
	{
		/*
	      Check if there is an empty slot, if so return index for that slot
	   */
		for (i = 0; i < pages.size(); i++)
			if (pages[i] == -1)
			{
				pages[i] = nextpage;
				p->push(nextpage);
				return i;
			}
		/*
	      Get the page to be replace, and find where it is stored in the pages vector
	   */
		int to_replace = p->pop();
		for (i = 0; i < pages.size(); i++)
		{
			if (pages[i] == to_replace)
				break;
		}

		/*
  	       update the queue for the new page
	   */

		p->push(nextpage);
	}

	return i;
}

/*
	Functions you need to implement (no need to uncomment these lines)

   int Page_Replacement_LRU(vector<int>& pages, int nextpage, PRDS_LRU* p)   {}
   int Page_Replacement_2nd(vector<int>& pages, int nextpage, PRDS_2nd* p)   {}
   int Page_Replacement_MyOwn(vector<int>& pages, int nextpage, PRDS_MyOwn* p) {}

*/
int main()
{
	int count = 3;
	vector<int> pagelist;
	vector<int> pages;
	PRDS_FIFO *prds = new PRDS_FIFO(count);
	int replace_count = 0;

	PRDS_LRU *prd1 = new PRDS_LRU(count);
	PRDS_2nd *prd2 = new PRDS_2nd(count);
	/*
     Uncomment the appropriate line(s) to test your program
   
     PRDS_MyOwn *prd3 = new PRDS_MyOwn(count);
   */

	srand(time(0));

	/*
    	initialize the buffer to be empty (-1)
   */
	for (int i = 0; i < count; i++)
		pages.push_back(-1);

	for (int i = 0; i < 20; i++)
		pagelist.push_back(rand() % 10);

	for (int i = 0; i < 20; i++)
	{
		//	Uncomment the appropriate lines to test your program

		// int res = Page_Replacement(pages, pagelist[i], prds);
		int res = Page_Replacement_LRU(pages, pagelist[i], prd1);
		// int res = Page_Replacement_2nd(pages, pagelist[i], prd2);
		//int res = Page_Replacement_MyOwn(pages, pagelist[i], prd3);

		if (res > -1)
		{
			replace_count++;
			pages[res] = pagelist[i];
		}
		cout << pagelist[i] << "  " << res << "  :  ";
		for (int i = 0; i < pages.size(); i++)
			cout << pages[i] << " ";
		cout << endl;
	}
	cout << "Page replaced count : " << replace_count << endl;
	return 0;
}
