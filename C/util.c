int clip(int x, int min, int max) {
  if (x <= min) {
    return min;
  } else if (x >= max) {
    return max;
  } else {
    return x;
  }
}
