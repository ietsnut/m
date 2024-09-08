import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class M {

    public static void main(String[] args) {

        try {
            Entity entity = new Entity(1, 1, 1, "main");
            ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
            scheduler.scheduleAtFixedRate(entity, 1000L, 1000L / 30, TimeUnit.MILLISECONDS);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }

    static class Entity extends Thread {

        final int[] self    = new int[4];
        final int[] entity  = new int[4];

        ProcessBuilder  builder;
        Process         process;
        OutputStream    out;
        InputStream     in;
        InputStream     err;

        public Entity(int scene, int shape, int state, String name) throws IOException {
            self[0] = 0;
            self[1] = scene;
            self[2] = shape;
            self[3] = state;
            this.builder = new ProcessBuilder(name + ".exe");
            builder.redirectErrorStream(true);
            this.process = builder.start();
            this.out     = process.getOutputStream();
            this.in      = process.getInputStream();
            this.err     = process.getErrorStream();
        }

        @Override
        public void run() {

            if (this.isInterrupted()) {
                try {
                    out.close();
                    in.close();
                    err.close();
                    process.destroy();
                    process.waitFor();
                } catch (IOException | InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }

            try {

                out();

                in();

            } catch (IOException e) {
                throw new RuntimeException(e);
            }

        }

        private void out() throws IOException {
            // Convert int[] to byte[], masking each int to treat it as unsigned
            byte[] send = new byte[] {
                    (byte) (self[0] & 0xFF),   // Start byte
                    (byte) (self[1] & 0xFF),   // Scene as unsigned byte
                    (byte) (self[2] & 0xFF),   // Shape as unsigned byte
                    (byte) (self[3] & 0xFF)    // State as unsigned byte
            };

            print("OUT: ", send);  // Print the sent bytes (for debugging)
            out.write(send);       // Write to output stream
            out.flush();           // Flush the output stream
        }

        private void in() throws IOException {

            int bytes = 0;
            byte[] buffer = new byte[4];

            while (bytes < 4) {
                int result = in.read(buffer, bytes, 4 - bytes);
                if (result == -1) {
                    break;
                }
                bytes += result;
            }

            for (int i = 0; i < 4; i++) {
                entity[i] = buffer[i] & 0xFF;
            }

            print("IN:  ", buffer);
            /*
            if (bytes == 4 && entity[0] == 0) {
                //System.out.println("IN:  " + Arrays.toString(entity));
            } else {
                //System.out.println("ERROR: " + Arrays.toString(entity));
            }*/

        }

    }

    private static void print(String prefix, byte... bytes) {
        StringBuilder sb = new StringBuilder(prefix);
        for (byte b : bytes) {
            String binary = String.format("%8s", Integer.toBinaryString(b & 0xFF)).replace(' ', '0');
            sb.append(binary).append(" (").append(b & 0xFF).append(") ");
        }
        System.out.println(sb.toString().trim());
    }

}
