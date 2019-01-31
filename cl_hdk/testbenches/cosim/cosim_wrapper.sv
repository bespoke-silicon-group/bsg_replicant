module cosim_wrapper();

   initial begin
      int exit_code;
      
      tb.power_up();

       $display("Hello\n");
       
      tb.test_main(exit_code);
      
      #50ns;

      tb.power_down();
      
      $finish;
   end

endmodule // cosim_wrapper
