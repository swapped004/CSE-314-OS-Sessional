#!/bin/bash

log_file_name="output.csv"
output_dir_name="1705025_output"


current_dir=""
output_dir=""
rel_dir=""
ignored=0

usage(){
	echo "Usage:"
	echo "./1705025.sh [-dname] -fname"
	echo "dname = Working Directory"
	echo "fname = Files containing the extensions to ignore"
}

update_log_file()
{
	echo "here"
	echo "$1"
	echo "$2"
	IFS=$'\n'  
	logger=( `cat "$1"` )
	echo "logger:"
	echo "${logger[*]}"

	echo "gap"

	line_no=0
	dupli=0

	for i in "${!logger[@]}";
	do
		#echo "${logger[$i]}"
		fname=( `echo "${logger[$i]}" | rev | cut -d '/' -f 1 | rev` )
		echo "$fname"
		line_no=$((line_no+1))

		#replace if there exists a file with the same name
		command_string=""$line_no"s#"${logger[$i]}"#$2#"
		# echo $command_string
		if [ "$fname" = "$3" ];then
			sed -i "$command_string" "$1"
			dupli=1
			break
		fi

	done

	#append if there is no duplicate
	if [ $dupli -eq 0 ];then
		echo "$2" >> $1
	fi

}

create_csv()
{
	touch "$log_file_name"
	printf "file_type\tno_of_files\n" > "$log_file_name"

	csv_file_path=`pwd`
	csv_file_path="$csv_file_path/$log_file_name"

	cd "$output_dir"

	for d in *
		do
			directory=$(basename -- "$d")
			cd "$directory"
			count=0
			for f in *
				do
					count=$((count+1))
				done
			cd ../
			#do not count log file
			count=$((count-1))
			echo "$directory"
			echo "$count"
			#add row to csv file
			printf "$directory\t$count\n" >> "$csv_file_path"
		done

	printf "ignored\t$ignored\n" >> "$csv_file_path"
	echo "$csv_file_path"
}

categorize_files()
{
	#now categorize the folders

	#move to the working directory
	cd "$1"

	#go through all the files recursively
	for f in *
	do
		if [ -d "$f" ];then
			categorize_files "$f"
		elif [ -f "$f" ];then
			file=$(basename -- "$f")

			#check if there is an extension
			if [[ "$file" == *"."* ]]; then

				#extract extension and filename
				extension="${file##*.}"
				filename="${file%.*}"
				echo "file_name: $filename -> ext: $extension"

				#check if ignored in given file
				ok=0
				for i in "${!array[@]}";
				 	do
				 		# echo "exT:$extension->ara:${array[$i]}"
				 		# echo "val:${array[$i]}->exT:$extension"
				 		if [ "$extension" = "${array[$i]}" ];then
				 			ok=1
				 			break
				 		fi
					done

				if [ $ok -eq 0 ];then
					echo "not ignored"

					#new directory
					temp_dir="$output_dir/$extension"
					#new file path
					new_path="$temp_dir/$file"
					#log file path
					desc_file_name="desc_"$extension".txt"
					log_file_path="$temp_dir/$desc_file_name"

					#make new directory if it doesn't exit already
					mkdir -v "$temp_dir"

					#current directory
					current_dir=`pwd`
					#current file path
					current_path="$current_dir/$file"

					echo "$file"
					echo "$current_path->$new_path"
					echo "$log_file_path"

					#copy file from working_dir to new_dir
					cp "$current_path" "$temp_dir"


					#update log.txt file
					touch "$log_file_path"
					#get rel_path
					repath=`realpath --relative-to="$rel_dir" "$current_path"`
					echo "relative path: $repath"
					#append to file
					update_log_file "$log_file_path" "$repath" "$file"
					#echo "$repath" >> $log_file_path
					
				else
					echo "Ignored"
					ignored=$((ignored+1))
				fi

			else 
				#file with no extentions. Send to other files.
				echo "no extentions"

				#new directory
				temp_dir="$output_dir/others"
				#new file path
				new_path="$temp_dir/$file"
				#log file path
				desc_file_name="desc_others.txt"
				log_file_path="$temp_dir/$desc_file_name"

				#make new directory if it doesn't exit already
				mkdir -v "$temp_dir"

				#current directory
				current_dir=`pwd`
				#current file path
				current_path="$current_dir/$file"

				echo "$file"
				echo "$current_path->$new_path"
				echo "$log_file_path"

				#copy file from working_dir to new_dir
				cp "$current_path" "$temp_dir"

				#update log.txt file
				touch "$log_file_path"
				#get rel_path
				repath=`realpath --relative-to="$rel_dir" "$current_path"`
				echo "relative path: $repath"
				#append to file
				update_log_file "$log_file_path" "$repath" "$file"
				#echo "$repath" >> $log_file_path
			fi
		fi
	done

	cd ../

	return 0
}


check_valid_file()
{
	echo "working directory: $1"

	#get relative path parent
	rel_dir=`pwd`
	echo "$rel_dir"

	#make the output_dir
	output_dir="$current_dir/"$1"/../"$output_dir_name""
	echo "$output_dir"
	mkdir -v "$output_dir"

	#check for valid ignore file
	if [ -f "$2" ];then
		file_name="$2"
		echo "valid input file -> $2"
	#else prompt the user for a valid input file 
	else
		file_name="$2"
		until [[ -f "$file_name" ]]; do
			echo "Invalid input file -> $file_name"
			echo "input a valid file name:"
			read file_name
		done
	fi

	#got valid file
	#save the ignored extensions in an array

	IFS=$'\r\n'
	array=( `cat "$file_name"` )

	#categorize all the files
	categorize_files "$1"

	#create the csv file
	create_csv
}


#main

current_dir=`pwd`
echo "$current_dir"

if [ $# -eq 1 ];then
	check_valid_file "." "$1" 

elif [ $# -eq 2 ];then
	check_valid_file "$1" "$2"

elif [ $# -eq 0 ];then
	echo "working directory: ."
	usage

else
	echo "more than two arguments"
fi

