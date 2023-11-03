export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa2";
export LD_PRELOAD="/home/sertitan/c_library_for_distributed_app/pa2/libruntime.so";
clang-3.5 -std=c99 -Wall -pedantic *.c -o pa2 -L. -lruntime

if [ "$#" -lt 2 ] || [ "$#" -gt 11 ]; then
  echo "Неверное количество аргументов. Использование: $0 <число от 2 до 10> [число1] [число2] ... [числоN]"
  exit 1
fi

count="$1"
if [ "$count" -lt 2 ] || [ "$count" -gt 10 ]; then
  echo "Неверное число. Введите число от 2 до 10."
  exit 1
fi

my_program=""

for ((i = 2; i <= count + 1; i++)); do
  arg="$2"
  if [ "$arg" -lt 1 ] || [ "$arg" -gt 99 ]; then
    echo "Число $arg не в допустимом диапазоне [1, 99]."
    exit 1
  fi
  my_program="$my_program$arg "
  shift
done

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/sertitan/c_library_for_distributed_app/pa2"; 

LD_PRELOAD=/home/sertitan/c_library_for_distributed_app/pa2/libruntime.so ./pa2 -p $count $my_program
