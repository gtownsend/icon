class labelgen : Sequence(prefix,postfix)
  method activate()
    return self.prefix||self$Sequence.activate()||self.postfix
  end
initially
  /(self.prefix) := ""
  /(self.postfix) := ""
  /(self.bounds)  := [50000]
end
