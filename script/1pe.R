#!/usr/bin/Rscript
# usage: $ ./1pe.R < R11065.1pe
dat<-read.table("stdin") # lines starting with # are ignored
x1 <-subset(dat, V6==1, select=c(V9,V10)) # amplification factor = 1 only
summary(x1) # print statistic summary of V9 (mean) and V10 (sigma)

png("1peMeanDist.png")
hist(x1$V9, breaks="Sturges",
     las=1, # rotate y label by 90 degree
     main="", # histogram title
     xlab="Mean of single PE distribution",
     ylab="Number of measurements")

png("1peSigmaDist.png")
hist(x1$V10, breaks="Sturges",
     las=1, # rotate y label by 90 degree
     main="", # histogram title
     xlab="Sigma of single PE distribution",
     ylab="Number of measurements")
