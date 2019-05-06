require "sinatra"
require "csv"

set :bind, '0.0.0.0'

post "/data" do
  puts params
end

get "/data", layout: false do
  csv = CSV.read("../log/data.csv")
  format = "%m/%d %H:%M"
  erb :data, content_type: :js, locals: {csv: csv, format: format}
end
