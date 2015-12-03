outfile = exists("outfile") ? outfile : "directed_degrees.pdf"
datafile = exists("datafile") ? datafile : "directed_degrees.dat"

print "Read data from '" . datafile . "' and output plot to '" . outfile . "'"
print "You can change these values using "
print "  gnuplot -e \"datafile='{path}';outfile='{path}'\" {script}\n"

set terminal pdf
set output outfile

set grid ytics lc rgb "#bbbbbb" lw 1 lt 0
set grid xtics lc rgb "#bbbbbb" lw 1 lt 0

set xlabel "Degree"
set ylabel "Multiplicity"

set logscale xy

plot \
   datafile index 0 using 1:2 with linespoints title "Out-Degree", \
   datafile index 1 using 1:2 with linespoints title "In-Degree"
