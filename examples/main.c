#include <bfu.h>
#include <stdio.h>

/* Needed so bfu_array_f(int) can auto-free the array on scope exit. */
DEFINE_BFU_ARRAY_DELETE(int)

int main(void) {
    /* bfu_string_f frees the string automatically when it goes out of scope. */
    bfu_string_f msg = bfu_string_new();
    bfu_string_push(&msg, "bfu_array has ");

    /* bfu_array_f frees the array automatically when it goes out of scope. */
    bfu_array_f(int) nums = NULL;
    bfu_array_push(nums, 10);
    bfu_array_push(nums, 20);
    bfu_array_push(nums, 30);

    bfu_string_push_format(&msg, "%lu items", bfu_array_size(nums));
    bfu_string_println(&msg);   /* bfu_array has 3 items */

    int last;
    bfu_array_pop(last, nums); /* last == 30, nums now has 2 items */

    /* String-keyed map. Items are passed by value, not by pointer. */
    bfu_map_str(int) counts = NULL;
    bfu_map_init(counts);
    bfu_map_insert(counts, "apples", 1);
    bfu_map_insert(counts, "oranges", 2);

    int n = 0;
    bfu_map_get(n, counts, "apples"); /* n == 1 */
    printf("apples => %d\n", n);

    bfu_map_delete(counts);

    bfu_process p = { .stdout_fd = bfu_process_fd_pipe() };
    bfu_process_run_sync(&p, "echo", "-n", "hello world");
    bfu_string_f out = {0};
    bfu_process_fd_read(&out, p.stdout_fd);
    bfu_process_close(&p);
    bfu_string_println(&out);

    p = (bfu_process){ .stdin_fd = bfu_process_fd_pipe(), .stdout_fd = bfu_process_fd_inherit() };
    bfu_process_run(&p, "cat", "-");
    bfu_process_fd_write(p.stdin_fd, &out);
    bfu_process_wait(&p);
    bfu_process_close(&p);
    return 0;
}
