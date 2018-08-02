
#include <assert.h>
#include <unistd.h>
#include <uv.h>

uv_fs_t open_req;
uv_buf_t iov;
char buffer[1024];

void on_open(uv_fs_t *req) {
    // The request passed to the callback is the same as the one the call setup
    // function was passed.
		printf("on open callback\n");
    assert(req == &open_req);
    if (req->result >= 0) {
        iov = uv_buf_init(buffer, sizeof(buffer));
    } else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}

void hare(void *arg) {
    int tracklen = *((int *) arg);
    while (tracklen) {
        tracklen--;
        sleep(1);
        fprintf(stderr, "Hare ran another step\n");
    }
    fprintf(stderr, "Hare done running!\n");
}

int counter = 0;

void one_time_callback(uv_timer_t *handle) {
  printf("one time timer triggered\n");
  counter++;

  if (counter > 20) {
    uv_timer_stop(handle);
  }
}

int counter2 = 0;

void another_time_callback(uv_timer_t *handle) {
  printf("another time timer triggered\n");
  counter2++;

  if (counter2 > 20) {
    uv_timer_stop(handle);
  }
}

void timefn(void *arg) {
	uv_timer_t timer_req;
	uv_timer_t timer_req2;
  uv_timer_init(uv_default_loop(), &timer_req);
	uv_timer_init(uv_default_loop(), &timer_req2);

  uv_timer_start(&timer_req, one_time_callback, 1000, 1000);
	uv_timer_start(&timer_req2, another_time_callback, 1000, 800);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_fs_req_cleanup(&open_req);
  uv_loop_close(uv_default_loop());
}

void file_test(char *path);

int main(int argc, char *argv[]) {
	int tracklen = 10;
	uv_thread_t hareid;
	uv_thread_create(&hareid, hare, &tracklen);

	uv_thread_t timerid;
	uv_thread_create(&timerid, timefn, NULL);

	//file_test(argv[1]);
	uv_fs_open(uv_default_loop(), &open_req, "./log.h", O_RDONLY, 0, on_open);

	uv_thread_join(&hareid);
	uv_thread_join(&timerid);
	return 0;
}



void file_test(char *path) {
	uv_fs_open(uv_default_loop(), &open_req, path, O_RDONLY, 0, on_open);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_fs_req_cleanup(&open_req);
}
