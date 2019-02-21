for((i=4;i<=5000;i=i+1))
do
  echo $i~5000 >> roundtimes
  for((j=1;j<=1000;j=j+1))
  do
    ./build/src/quic/examples/ns3.27-quic-mapreduce-debug --round=$i
  done
done