export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa5";
export LD_PRELOAD="/home/sertitan/c_library_for_distributed_app/pa5/libruntime.so";
clang-3.5 -std=c99 -Wall -pedantic *.c -o pa5 -L. -lruntime

if [ "$#" -lt 1 ] || [ "$#" -gt 3 ]; then
  echo "Неверное количество аргументов. Использование: $0 -p <число от 2 до 10> --mutexl или $0 --mutexl -p <число от 2 до 10>"
  exit 1
fi

opt="$1 "
if [[ $opt == "--mutexl " ]]; then
  opt2="$2 "
  count="$3"
  if [[ $opt2 == "-p " ]]; then
    if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
      echo "Неверное число. Введите число от 2 до 10."
      exit 99
    else
      str="$opt $opt2 $count"
    fi
  else
    exit 100
  fi
  
elif [[ $opt == "-p " ]]; then
  count="$2"
  if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
    echo "Неверное число. Введите число от 2 до 10."
    exit 101
  fi
  opt2="$3"
  if [[ $opt2 == "--mutexl" ]]; then
    str="$opt $count $opt2"
  else
    str="$opt $count"
  fi
else
  exit 102
fi

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa5"; 
LD_PRELOAD=/home/sertitan/c_library_for_distributed_app/pa5/libruntime.so ./pa5 $str

