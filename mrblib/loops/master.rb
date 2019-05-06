debugprint('start', 'main_loop')

$co2 = Co2.new
$thermistor = Thermistor.new

led = Led.new(19)

while true
  co2 = $co2.concentrate
  temperature = $thermistor.temperature
  puts "CO2: #{co2}, Temperature: #{temperature}"
  if co2 > 2000
    5.times do
      led.turn_on
      sleep 0.1
      led.turn_off
      sleep 0.1
    end
  elsif co2 > 1500
    led.turn_on
    sleep 1
  else
    led.turn_off
    sleep 1
  end
end
