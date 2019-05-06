require "charty"
require "csv"
require "time"

# when you want to use matplotlib
charty = Charty.new(:matplot)
#charty = Charty.new(:gruff)

csv = CSV.read("co2.csv")
time_array = []
value_array = []
csv[(csv.size - 10)..(csv.size - 1)].each do |data|
  time_array << Time.parse(data[0]).strftime("%H:%M:%S")
  value_array << data[1]
end

curve2 = charty.curve do
  series time_array, value_array, label: "CO2"
  range x: 0..10, y: 1000..2000
  xlabel 'Time'
  ylabel 'CO2'
end
curve2.render("curve_matplot.png")
