sleep 80 # wait until CO2 sensor is warmed up

debugprint('start', 'sub_loop')

while true
  co2 = $co2.concentrate
  temperature = $thermistor.temperature
  if co2 > 0
    data = "co2=#{co2}&temperature=#{temperature}"
    puts "DATASEND:#{data}"
    debugprint("slave_loop", "debug")
    sleep 300
  else
    sleep 3
  end
end
