class Cartesian : Radian (x,y)
initially
  if /(self.r) then {
    self.r := sqrt(self.x^2+self.y^2)
    self.d := 0 # this should really be some awful mess
  }
end
class Radian : Cartesian(d,r)
initially
  if /(self.x) then {
    self.x := 0
    self.y := 0
  }
end
