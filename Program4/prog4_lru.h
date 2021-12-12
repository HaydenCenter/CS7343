#include <vector>
#include <deque>
#include <algorithm>

using namespace std;

class PRDS_LRU
{
public:
    PRDS_LRU(int pages) { max = pages; } // Sets the max number of pages in main memory
    deque<int> pages;                    // Stores the three most recently used pages in order of least (front) to most (back).

    // Takes in the next page as a parameter and returns the an integer to tell the page replacement function what to do.
    // result == -2: Do not replace anything
    //        == -1: Replace first empty spot
    //        >=  0: Replace page <result>
    int replaceWith(int page)
    {
        auto recent = find(pages.begin(), pages.end(), page); // Find if the page already exists in memory

        int result;
        if (recent != pages.end()) // If yes, remove the page from the queue and replace it at the back.
        {
            result = -2; // Return -2 to tell the page replacement function to not replace anything.
            pages.erase(recent);
        }
        else if (pages.size() < max) // If no and memory is not full, place in an "empty" spot.
        {
            result = -1; // Return -1 to tell the page replacement function to replace an "empty" page.
        }
        else // Otherwise, replace the least recently used page.
        {
            result = pages.front();
            pages.pop_front();
        }
        pages.push_back(page); // Add the new page to the back of the queue.
        return result;
    }

private:
    int max;
};

int Page_Replacement_LRU(vector<int> &pages, int nextpage, PRDS_LRU *p)
{
    int replace = p->replaceWith(nextpage); // Find value of page to be replaced
    if (replace < -1)                       // If less than -1, do nothing.
        return -1;

    // Replace page in vector
    for (int i = 0; i < pages.size(); i++)
    {
        if (pages[i] == replace)
        {
            return i;
        }
    }

    return -1;
}