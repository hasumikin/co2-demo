require "file-tail"
require "csv"
require 'fileutils'

csvfile = "data.csv"
FileUtils.touch(csvfile)

filename = 'log.txt'

# ENV["TZ"] = "Europe/Warsaw" # 中央ヨーロッパ時間
ENV["TZ"] = "America/Indiana/Knox" # CST

File.open(filename) do |log|
  log.extend(File::Tail)
  log.interval = 0
  log.backward(0)
  log.tail do |line|
    puts line
    if data = line.match(/\ADATASEND:(.+)/)
      CSV.open(csvfile, "a") do |csv|
        sensors = data[1].split("&").map{|sensor| sensor.split("=")}
        csv << [Time.now, sensors[0][1].chomp, sensors[1][1].chomp]
      end
    end
  end
end

