print "Rounds: ";
var rounds = readNumber();

var start = clock();

var x = 1.0;
var pi = 1.0;

for (var i = 2; i < rounds + 2; i = i+1) {
    x = -x;
    pi = pi + x / (2 * i - 1);
}

pi = pi * 4;
var end = clock();

print pi;
print "\n";
print end-start;