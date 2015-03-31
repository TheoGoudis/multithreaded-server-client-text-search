#!/bin/bash
#by Theo Goudis

if [ ! -d ./samples ]; then
	echo "Create a directory called samples"
	mkdir ./samples
fi

if [ -f ./f1.txt ]; then
	echo "Moving f*.txt files in to ./samples"
	mv ./f*.txt ./samples
elif [ ! -f ./samples/f1.txt ]; then
	echo "There are no files to be searched, exiting."
	sleep 3
	exit 0
fi

echo "Executing make all"
make all
sleep 2

while :
do
	clear

        echo "Server Name - $(hostname)"
	echo "-------------------------------"
	echo "      TEST OUR PROGRAMM        "
	echo "-------------------------------"
	echo "1. run ./server &"
	echo "2. run ./client"
	echo "3. run ./client localhost"
	echo "4. Search for one word"
	echo "5. Search for 10 words"
	echo "6. Search for 15 words"
	echo "7. send SIGSTOP to server"
	echo "8. send SIGINT to server"
	echo "9. exit bash script"

	read -p "Enter your choice [ 1 - 9 ] " choice
	
	case $choice in
		1)
			./server &
			spid=$!
			sleep 1
			read -p "[Enter]" readEnterKey
			;;
		2)
			./client
			read -p "[Enter]" readEnterKey
			;;
		3)
			./client localhost
			read -p "[Enter]" readEnterKey
			;;
		4) 
			echo "./client localhost Agatha"
			./client localhost Agatha
			sleep 1
			read -p "[Enter]" readEnterKey
			;;
		5)
			echo "./client localhost you give the monkey ten bananas he wants some more"
			./client localhost you give the monkey ten bananas he wants some more
			sleep 2
			read -p "[Enter]" readEnterKey
			;;
		6)
			echo "./client localhost you gave the monkey too many bananas at least give him a soda"
			./client localhost you gave the monkey too many bananas at least give him a soda
			sleep 1
			read -p "[Enter]" readEnterKey
			;;
		7)
			kill -SIGTSTP $spid
			sleep 1
			read -p "[Enter]" readEnterKey
			;;
		8)
			kill -SIGINT $spid
			sleep 1
			read -p "[Enter]" readEnterKey
			;;
		9)
			kill -2 $spid
			exit 0
			;;
		*)
			echo "Error: Invalid option..."	
			read -p "[Enter]" readEnterKey
			;;
	esac		
 
done
