{* Turn a LED on or off *}

program led;

const
  INPUT = 0;
  OUTPUT = 1;

var
  pin, value: Integer;

begin
  write('Enter pin number: ');
  readln(pin);
  write('Enter value (0 = LOW, 1 = HIGH): ');
  readln(value);
  Bdos(220, (pin shl 8) + OUTPUT);
  Bdos(222, (pin shl 8) + value);
end.
