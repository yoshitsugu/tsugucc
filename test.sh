#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./tsugucc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 1 '(-33+35)/2;'
assert 1 '(2 * 3) <= 6;'
assert 0 '(2 * 3) < 6;'
assert 4 'a = 1;b = 3;return a + b;3 * 3;'
assert 4 'abc = 100;b2 = 2;cde = abc = 1;abc + b2 + cde;'
assert 3 'if (1-1) return 1; else return 3;'
assert 1 'if (1) return 1; else return 3;'
assert 3 'if (0) return 1; return 3;'
assert 20 'i=0; while( i<20 ) i = i + 1;return i;'
assert 11 'a = 1;for(i=0;i<5;i=i+1) a = a + i; return a;'
assert 5 'i = 0;for(;i<5;) i = i + 1; return i;'
assert 4 '{ 1; {  3; } return 4; } '
assert 8 'if (0) { a = 1; b = 2; } else { a = 3; b = 5;} return a + b;'

echo OK