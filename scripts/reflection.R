# generate a reflex / rotate / mirror matrix
# there are almost certainly library functions to do this, but i did this as an exercise
# note that this is left over from a 2d exercise, needs to be converted for 3d

# rotate by 60 values
sin60=sqrt(3)/2
cos60=1/2

# translate by 3,2 values
transX=3
transY=2

t1 <- matrix(c(1, 0, transX, 0, 1, transY, 0, 0,1 ), nrow = 3, ncol = 3, byrow = TRUE)
t2 <- matrix(c(1, 0, -transX, 0, 1, -transY, 0, 0,1 ), nrow = 3, ncol = 3, byrow = TRUE)

r1 <- matrix(c(cos60, -sin60, 0, sin60, cos60, 0, 0, 0,1 ), nrow = 3, ncol = 3, byrow = TRUE)
r2 <- matrix(c(cos60, sin60, 0, -sin60, cos60, 0, 0, 0,1 ), nrow = 3, ncol = 3, byrow = TRUE)

mirrorY <- matrix(c(1, 0, 0, 0, -1, 0, 0, 0,1 ), nrow = 3, ncol = 3, byrow = TRUE)

# %*% is the matrix multiply operator
# i think all the parens are redundant here - wanted to be sure
R = (t2 %*% (r2 %*% (mirrorY %*% (r1 %*% t1))))

R 

# create a vector
v = c(1,2,1)

res = R %*%  v

