+ [x] when the player makes a move that eliminates a piece, the Arduino responds
with the parsed coordinates of the elimination move, which are used to make a move
on the sunfish matrix, which obviously fails, causing a crash
+ [ ] fix moving algorithm
  + [x] when eliminating on second white row, near the border, the algorithm goes against the "wall"
  + [ ] strange movements sometimes happen
+ [x] arduino should report queue empty only when both queues are empty, otherwise
when eliminating a piece (two movements) the host thinks the move is done only half-way
+ [x] fix human vs human because of <pos> being None in the methods and throwing
exceptions everywhere
+ [ ] there is some memory leak in the python script, highly noticeable when running *computer vs computer*
