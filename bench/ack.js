function ack(m, n) {
  const stack = [];
  stack.push({ m, n });

  while (stack.length > 0) {
    const frame = stack.pop();
    m = frame.m;
    n = frame.n;

    if (m === 0) {
      if (stack.length === 0) return n + 1;
      stack[stack.length - 1].n = n + 1;
    } else if (n === 0) {
      stack.push({ m: m - 1, n: 1 });
    } else {
      stack.push({ m: m - 1, n: null });
      stack.push({ m: m, n: n - 1 });
    }
  }
}

console.log(ack(3, 10));
