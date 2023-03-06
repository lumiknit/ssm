# ssml: ML Dialect(?) for SSM

ML for SSM

## Code example with description

### Basic

```text
# Line comment starts with hash `#`

# `=` is a binding symbol
a = 20
b = 42.1

# `:` for type annotations

c: Int
c = 42

# `=>` for function type and use `\` to create function
inc: Int => Int
inc = \x = x + 1

# Otherwise, you can put arguments on lhs
dec: Int => Int
dec x = x - 1

# Function can be called by listing arguments, as ML
two = inc 1
three = dec 4

# Multi-variable function
fadd: Flt => Flt => Flt
fadd x y = x + y

seven = fadd 3.0 4.0
```

### Types

```text
# Basis types:
a: Int # Integer number of at least 30-bit data and a sign bit
b: Flt # Float number of 32-bit with zero at lsb
c: Bytes # Byte array
p: Ptr # Pointer aligned by 2

# (Short) tuple
# (,,,) to create tuples
a: (Int, Int)
a = (1, 2)

# Variable of name starts with `_` is used for unused variables
_ = 3
# Type of name starts with `_` is used for type variable without constraints
a: (_T, b)
a = (1, 2)
```

### ADT & Pattern Match

```text
# To create ADT, use `?` to create a type
# Built-in bool
Bool:? False
     | True

Maybe a:? None
        | Just a

Either l r:? Left l
           | Right r

# Symbol of list is `[]`
[a]:? Nil
    | Cons a [a]

# Pattern matching
# `?` is pattern operator in value space
# `|` means case and `=>` means then

len: [_elem] => Int
len xs = xs ? Nil => 0
            | Cons _ xs' => 1 + len xs'

# For convinent, you can omit lhs of `?`.

boolToInt: Bool => Int
boolToInt =? False => 0 | True => 1
# In this case, the RHS is equal to
# \x = x ? False => 0 | True => 1

# Note that the below codes are equivalent:
a x = x? False => 0 | True => 1
a x = (? False => 0 | True => 1) x
a = \x => x? False => 0 | True => 1
a = \x => (? False => 0 | True => 1) x
a = ? False => 0 | True => 1
```

# Others

```text
# Also, RHS is a kind of function body
normOfRandomVector: () => Int
normOfRandomVector () =
  x = randInt 0 100
  y = randInt 0 100
  x*x + y*y

# Operator can be identifier using backquote
`+`: Int => Int => Int
`+` x y = @add x y

# `@ID` is a magics, which can be implemented using SSML
# such as I/O, etc. It is provided by SSM runtime.

# Syntatic sugar for operator
# For example, we can write below app operator
`->` x f = f x
# Then, usually it can be used as,
f x =
  x * x -> \y =
  x * 20 + y
# However, as this looks ugly, we provide other way to write it.
f x =
  \y <- x * x
  x * 20 + y

# \a1 a2 ... an <- body; e is equivalent to body -> \a1 a2 ... an; e
# Note that operator is reversed
# This is useful for monadic I/O:

main = getLine >>= \line = putStrLn line
main =
  \x =<< getLine
  putStrLn x

# Or let-binding
`->` x f = f x
main =
  \a <- 20
  \b <- 30
  a + b

# Typeclass

show a: a => Str
# Then, a type is an instance of `show` if the above value is implemented

# To implement typeclass,
: show Int
show i = @intToStr i

identity t: t
unity t: t
add t: t => t => t
mul t: t => t => t
Ring t: identity t
      | unity t
      | add t
      | mul t

: identity Int
identity = 0
: unity Int
unity = 1
: add Int
add x y = x + y
: mul Int
mul x y = x * y
# Then, Int is a ring

`>>=` m: m a => (a => m b) => m b
return m: a => m a
Monad m: `>>=` m
       | return m

```

### Modules

```text
# Include other file
@"abc/def" # Absolute path
@"./abc/def" # Relative path

# Current file module
@.main

@+

# Operator prefix
@< `+` 20 # Left associative
@> `$` 20 # Right associative


```