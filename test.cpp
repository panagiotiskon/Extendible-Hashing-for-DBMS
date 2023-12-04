#include <bits/stdc++.h>
using namespace std;

// Function to extract k bits from p position
// and returns the extracted value as integer
int bitExtracted(int number, int k, int p)
{
    return (((1 << k) - 1) & (number >> (p - 1)));
}

// Driver code
int main()
{
    int number = 21312, k = 3, p =13;
    cout << "The extracted number is " << bitExtracted(number, k, p);

    return 0;
}
