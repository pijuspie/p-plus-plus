print "Number n: ";
var n = readNumber();

fun fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2); 
}

print "The n-th element of the Fibonacci sequence: ";
var before = clock();
print fib(n);
var after = clock();
print "\nDuration: ";
print after - before;