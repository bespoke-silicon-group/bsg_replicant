# statically computed values
log2 = $(shell echo 'l($(1))/l(2)' | bc -l | xargs printf "%.f\n")
