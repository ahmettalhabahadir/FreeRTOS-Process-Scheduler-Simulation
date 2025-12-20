all: freertos_sim

# --- BAĞLAMA (LINKING) ---
freertos_sim: lib/main.o lib/scheduler.o lib/tasks.o lib/freertos_hooks.o lib/freertos_tasks.o lib/freertos_queue.o lib/freertos_list.o lib/freertos_timers.o lib/freertos_event_groups.o lib/freertos_stream_buffer.o lib/freertos_port.o lib/freertos_heap.o lib/freertos_utils.o
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. lib/main.o lib/scheduler.o lib/tasks.o lib/freertos_hooks.o lib/freertos_tasks.o lib/freertos_queue.o lib/freertos_list.o lib/freertos_timers.o lib/freertos_event_groups.o lib/freertos_stream_buffer.o lib/freertos_port.o lib/freertos_heap.o lib/freertos_utils.o -lrt -o freertos_sim

# --- DERLEME (COMPILING) - KENDİ DOSYALARIN ---

lib/main.o: src/main.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c src/main.c -o lib/main.o

lib/scheduler.o: src/scheduler.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c src/scheduler.c -o lib/scheduler.o

lib/tasks.o: src/tasks.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c src/tasks.c -o lib/tasks.o

lib/freertos_hooks.o: src/freertos_hooks.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c src/freertos_hooks.c -o lib/freertos_hooks.o

# --- DERLEME (COMPILING) - FREERTOS DOSYALARI ---

lib/freertos_tasks.o: FreeRTOS/tasks.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/tasks.c -o lib/freertos_tasks.o

lib/freertos_queue.o: FreeRTOS/queue.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/queue.c -o lib/freertos_queue.o

lib/freertos_list.o: FreeRTOS/list.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/list.c -o lib/freertos_list.o

lib/freertos_timers.o: FreeRTOS/timers.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/timers.c -o lib/freertos_timers.o

lib/freertos_event_groups.o: FreeRTOS/event_groups.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/event_groups.c -o lib/freertos_event_groups.o

lib/freertos_stream_buffer.o: FreeRTOS/stream_buffer.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/stream_buffer.c -o lib/freertos_stream_buffer.o

lib/freertos_port.o: FreeRTOS/portable/ThirdParty/GCC/Posix/port.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/portable/ThirdParty/GCC/Posix/port.c -o lib/freertos_port.o

lib/freertos_heap.o: FreeRTOS/portable/MemMang/heap_4.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/portable/MemMang/heap_4.c -o lib/freertos_heap.o

lib/freertos_utils.o: FreeRTOS/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
	mkdir -p lib
	gcc -Wall -Wextra -g -O0 -pthread -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/ThirdParty/GCC/Posix -I. -c FreeRTOS/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c -o lib/freertos_utils.o

clean:
	rm -rf lib
	rm -f freertos_sim