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
            scheduler.scheduleAtFixedRate(entity, 1000L, 1000L / 8, TimeUnit.MILLISECONDS);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }

    static class Entity extends Thread {

        final byte scene, shape, state;
        final byte[] entity = new byte[4];

        ProcessBuilder  builder;
        Process         process;
        OutputStream    out;
        InputStream     in;
        InputStream     err;

        public Entity(int scene, int shape, int state, String name) throws IOException {
            this.scene = (byte) scene;
            this.shape = (byte) shape;
            this.state = (byte) state;
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

                byte[] send = new byte[] { 0, scene, shape, state };
                System.out.println("OUT: " + Arrays.toString(send));

                out.write(send);
                out.flush();
                int bytes = 0;
                while (bytes < 4) {
                    int result = in.read(entity, bytes, 4 - bytes);
                    if (result == -1) {
                        System.out.println("ERROR");
                        break;
                    }
                    bytes += result;
                }

                if (bytes == 4 && entity[0] == 0) {
                    System.out.println("IN:  " + Arrays.toString(entity));
                } else {
                    System.out.println("ERROR: " + Arrays.toString(entity));
                }

            } catch (IOException e) {
                throw new RuntimeException(e);
            }

        }

    }

}
