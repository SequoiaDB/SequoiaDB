
import com.sequoias3.SequoiaS3;
import com.sequoias3.SequoiaS3ClientBuilder;

public class testmain {
    private SequoiaS3 regionClient;

    String accessKey="ABCDEFGHIJKLMNOPQRST";
    String secretKey="abcdefghijklmnopqrstuvwxyz0123456789ABCD";

    String endPoint = "http://localhost:8002";

    public testmain(){
        try{
            SequoiaS3 region = SequoiaS3ClientBuilder.standard()
                    .withEndpoint(endPoint)
                    .withAccessKeys(accessKey, secretKey)
                    .build();

            region.getRegion("region-example");
        }catch (Exception e){
            ;
        }
    }
}
