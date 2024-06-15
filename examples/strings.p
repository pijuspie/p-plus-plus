var x = 85.43899;
var y = 100;
for (var i = 0; i<9; i = i+1) {
    print "x: " + stringify(round(x, y)) + "\n";
    y = y / 10;
}