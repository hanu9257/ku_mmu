RANDOM=$$
DIV=$((64))
if [ $# -ne 2 ]; then
	echo "Usage:\t$0 PROC# EXAMPLE#"
	exit 1
fi
rm test.txt
for i in `seq $2`
do
	PROC=$(($RANDOM%$1+1))
	VN=$(($RANDOM%DIV+1))
	echo "$PROC $VN" >> test.txt
done
