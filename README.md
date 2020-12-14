Uses dynamic programming to get the best possible score for any deal.

# Usage
Manually create an input file input.txt with the four card piles (listed in order from top to bottom).
Example input:
```
10 6 8 6 3 7 5 10 10 J Q A K
K 3 A 8 3 4 5 6 3 Q A 2 9
2 J 5 K A Q 7 8 7 2 5 7 9
J J 2 4 9 4 Q K 8 9 10 4 6
```

Compile the program:
```
g++ -std=c++17 -O3 -Wall -o cribbage
```

Run the program. This takes around 20 seconds:
```
./cribbage < I.0 |& tee out
```

Follow the directions:
```
less -RS out
```

Example output:
```
Best possible score: 106
Play card from stack 1 (which is a 13) scoring 0 total=0
Play card from stack 1 (which is a 1) scoring 0 total=0
Play card from stack 1 (which is a 12) scoring 0 total=0
Play card from stack 2 (which is a 9) scoring 0 total=0
NEW STACK
Play card from stack 1 (which is a 11) scoring 2 total=2
Play card from stack 1 (which is a 10) scoring 0 total=2
Play card from stack 1 (which is a 10) scoring 2 total=4
NEW STACK
Play card from stack 1 (which is a 5) scoring 0 total=4
Play card from stack 4 (which is a 6) scoring 0 total=4
Play card from stack 4 (which is a 4) scoring 5 total=9
Play card from stack 1 (which is a 7) scoring 4 total=13
Play card from stack 1 (which is a 3) scoring 5 total=18
<more lines>
```
