function shuffle_loop(n, a, b, c, d, e) {
  let res = e;
  for (let i = 0; i < n; i++) {
    // 模拟参数重排
    const na = a - 1;
    const nb = e;
    const nc = d;
    const nd = c;
    const ne = b;
    a = na; b = nb; c = nc; d = nd; e = ne;
    res = e;
  }
  return res;
}

console.log(shuffle_loop(40_000_000, 40_000_000, 1, 2, 3, 4));
