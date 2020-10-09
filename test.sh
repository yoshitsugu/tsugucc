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
assert 4 'a = 1;b = 3;a + b;'
assert 4 'abc = 100;b2 = 2;cde = abc = 1;abc + b2 + cde;'

echo OK