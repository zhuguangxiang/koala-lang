SET (LIBTASK_SRC
			task.c task_scheduler.c task_context.c locked_linked_deque.c
		)

ADD_LIBRARY(task SHARED ${LIBTASK_SRC})
target_link_libraries (task pthread)
set_target_properties(task PROPERTIES VERSION 0.5 SOVERSION 0)
