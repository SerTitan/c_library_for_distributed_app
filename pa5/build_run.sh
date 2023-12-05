export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa4";
export LD_PRELOAD="/home/sertitan/c_library_for_distributed_app/pa4/libruntime.so";
clang-3.5 -std=c99 -Wall -pedantic *.c -o pa4 -L. -lruntime

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Неверное количество аргументов. Использование: $0 <флаги> <число от 2 до 10>"
  exit 1
fi

opt="$1 "
if [[ $opt != "--mutexl " ]]; then
  opt=""
  count="$1"
  if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
    echo "Неверное число. Введите число от 2 до 10."
    exit 1
  fi
else
  count="$2"
  if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
    echo "Неверное число. Введите число от 2 до 10."
    exit 1
  fi
fi


export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa4"; 

LD_PRELOAD=/home/sertitan/c_library_for_distributed_app/pa4/libruntime.so ./pa4 $opt-p $count
