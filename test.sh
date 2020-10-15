#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
EOF

assert() {
  expected="$1"
  input="$2"

  ./tsugucc "$input" > tmp.s
  cc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 21 "main () { 5+20-4; }"
assert 41 "main () { 12 + 34 - 5 ; }"
assert 15 "main() { 5*(9-6); }"
assert 4  "main() { (3+5)/2; }"
assert 1  "main() { (-33+35)/2; }"
assert 1  "main() { (2 * 3) <= 6; }"
assert 0  "main() { (2 * 3) < 6; }"
assert 4  "main() { int a; int b; a = 1;b = 3;return a + b;3 * 3; }"
assert 4  "main() { int abc; int b2; int cde; abc = 100;b2 = 2;cde = abc = 1;abc + b2 + cde; }"
assert 3  "main() { if (1-1) return 1; else return 3; }"
assert 1  "main() { if (1) return 1; else return 3; }"
assert 3  "main() { if (0) return 1; return 3; }"
assert 20 "main() { int i; i=0; while( i<20 ) i = i + 1;return i; }"
assert 11 "main() { int i; int a; a = 1;for(i=0;i<5;i=i+1) a = a + i; return a; }"
assert 5  "main() { int i; i = 0;for(;i<5;) i = i + 1; return i; }"
assert 4  "main() { { 1; {  3; } return 4; }  }"
assert 8  "main() { int a; int b; if (0) { a = 1; b = 2; } else { a = 3; b = 5;} return a + b; }"
assert 3  "main() { return ret3(); }"
assert 21 "main() { add6(1, 2, 3, 4, 5, 6); }"
# assert 5  "return3() { return 3; } main() { return return3() + 2; }"
# assert 6  "add3(int a, int b, int c) { return a + b + c; } main() { return add3(1,2,3); }"
assert 8 "main() { int a; int b; b = 8; a = &b; return *a; }"
assert 3 "main() { int x; int *y; y = &x; *y = 3; return x; }"

echo OK
