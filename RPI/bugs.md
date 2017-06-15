+ when the player makes a move that eliminates a piece, the Arduino responds
with the parsed coordinates of the elimination move, which are used to make a move
on the sunfish matrix, which obviously fails, causing a crash

+ Fix moving algorithm
  + When eliminating on second white row, near the border, the algorithm goes against the "wall"
  + 
