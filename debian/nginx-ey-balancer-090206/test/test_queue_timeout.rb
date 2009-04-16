require File.dirname(__FILE__) + '/maxconn_test'

backends = []
1.times { backends << MaxconnTest::DelayBackend.new(5) }
test_nginx(backends,
  :max_connections => 1, # per backend, per worker
  :worker_processes => 1,
  :queue_timeout => "1s"
) do |nginx|
  out = %x{httperf --num-conns 200 --hog --timeout 100 --rate 20 --port #{nginx.port}}
  assert $?.exitstatus == 0
  results = httperf_parse_output(out)
  #assert_equal 100, results["5xx"]
end

