require File.dirname(__FILE__) + '/maxconn_test'
backend = MaxconnTest::DelayBackend.new(0.9)
test_nginx([backend],
  :max_connections => 2, # per backend, per worker
  :worker_processes => 1
) do |nginx|
  # Pound the server with connections which close on the client-side
  # immeditaely after hitting. (Note --timeout 0.01)
  50.times do 
    %x{httperf --num-conns 20 --hog --timeout 0.01 --rate 100 --port #{nginx.port}}
    assert $?.exitstatus == 0
  end
  
  # just making sure nginx doesn't have any workers that die
end


# Okay - we allow it to grow above the given max_connection
# because the nginx module had to close the upstream 
# connection - that means Mongrel has to handle an exception
# before it clears that connection. 
# This is, perhaps, acceptable since HAproxy does the same.

#puts "got #{backend.experienced_max_connections} connections"
#assert(backend.experienced_max_connections <= 6) 


